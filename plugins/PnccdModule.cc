#include "ami/plugins/PnccdModule.hh"
#include "ami/data/Cds.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryScan.hh"
#include "ami/event/FrameCalib.hh"
#include "ami/event/PnccdCalib.hh"
#include "pdsdata/xtc/TypeId.hh"

//#define DBUG

static const unsigned rows_segment = 512;
static const unsigned cols_segment = 512;
static const unsigned rows = 2*rows_segment;
static const unsigned cols = 2*cols_segment;
static const unsigned Offset = 1024;
static const int nrg_max   = 16000;

static int nrg_bins  = (1<<14)/10;
static int nrg_range = (1<<14)-256;
static int nrg_min   = -256;
static int ped_events= 100;
static int ped_reject=   4;

static Pds::ClockTime __clk(1,0);

namespace Ami {
  class PedCalib {
  public:
    PedCalib()
    { 
      reset(); 
    }
    ~PedCalib() {}
  public:
    void reset() {
      for(unsigned row=0; row<rows_segment; row++)
	for(unsigned col=0; col<cols_segment; col++) {
	  _offsets[row][col].clear();
	  _sum    [row][col] = 0;
	}
    }
    void init  (int row, const int* v) {
      for(unsigned col=0; col<cols_segment; col++) {
	_offsets[row][col].push_back(v[col]);
	_offsets[row][col].sort();
      }
    }
    void accum (int row, const int* w) {
      for(unsigned col=0; col<cols_segment; col++) {
	int v = w[col];
	std::list<int>& l = _offsets[row][col];
	if (v < l.front())
	  _sum[row][col] += v;
	else {
	  _sum[row][col] += l.front();
	  l.pop_front();
	  if (v > l.back())
	    l.push_back(v);
	  else
	    for(std::list<int>::iterator it=l.begin();
		it!=l.end(); it++)
	      if (v < *it) {
		l.insert(it,v);
		break;
	      }
	}
      }
    }
    int accumulated(int col, int row) const { return _sum[row][col]; }
  private:
    std::list<int> _offsets[rows_segment][cols_segment];
    int            _sum    [rows_segment][cols_segment];
  };

  class PnccdModule::ModulePlots {
  public:
    ModulePlots(const Pds::DetInfo&, void* payload);
    ~ModulePlots();
  public:
    void analyze(void* payload);
    void add   (Cds*);
    void remove(Cds*);
    void reset ();
  private:
    void _fillQuadrant (const Pds::PNCCD::FramesV1&, unsigned iq);
    void _analyze_droplets(ndarray<int32_t,2> v,
			   ndarray<const uint16_t,2> d,
			   unsigned           iq,
			   double             nrg_cut,
			   double             nbr_cut);
  public:
    Pds::DetInfo       _info;
    Pds::PNCCD::ConfigV2 _config;
    ndarray<uint16_t,2>  _ped[4];
    PedCalib             _ped_calib[4];
    int _n;
    int _common_lo;
    int _common_hi;
    unsigned* _common_dist;
    EntryImage* _dist;
    EntryTH1F*  _enrg   [4];
    EntryTH1F*  _enrg_ctr[4];
    EntryTH1F*  _enrg_sum[4];
    EntryTH1F*  _enrg_sum_vm[64];
    EntryTH1F*  _enrg_1p[4];
    EntryTH1F*  _enrg_2p[4];
    EntryTH1F*  _enrg_3p[4];
    EntryTH1F*  _enrg_4p[4];
    EntryTH1F*  _comb   [4];
    EntryProf*  _erat   [4];
    EntryTH1F*  _erat_2p[4];
    EntryTH1F*  _erat_3p[4];
    EntryTH1F*  _erat_4p[4];
  };

};

Ami::PnccdModule::PnccdModule() : _cds(0), _clk(0,0), _changed(false) {}
Ami::PnccdModule::~PnccdModule() {}

void Ami::PnccdModule::reset    (Ami::FeatureCache&) 
{
}

void Ami::PnccdModule::clock    (const Pds::ClockTime& clk) 
{
  if (_changed) {
    _changed=false;
    //    recreate();
  }
  __clk=clk; 
}

void Ami::PnccdModule::configure(const Pds::DetInfo&   src,      // configuration data callback
				 const Pds::TypeId&    type,
				 void*                 payload)
{
  if (type.id() == Pds::TypeId::Id_pnCCDconfig) {
    for(std::list<ModulePlots*>::iterator it=_plots.begin();
	it!=_plots.end(); it++)
      if ((*it)->_info == src) {
	(*it)->reset();
	return;
      }
    _plots.push_back(new ModulePlots(src,payload));
    _changed=true;
  }
}

void Ami::PnccdModule::event    (const Pds::DetInfo&   src,      // event data callback
				 const Pds::TypeId&    type,
				 void*                 payload)
{
  if (!_cds) return;

  if (type.id() == Pds::TypeId::Id_pnCCDframe) {
    for(std::list<ModulePlots*>::iterator it=_plots.begin();
	it!=_plots.end(); it++) {
      if (src == (*it)->_info) {
	(*it)->analyze(payload);
	break;
      }
    }
  }
}

void Ami::PnccdModule::clear    ()
{
  if (!_cds) return;

  for(std::list<ModulePlots*>::iterator it=_plots.begin();
      it!=_plots.end(); it++) {
    (*it)->remove(_cds);
  }
  _cds = 0;
}

void Ami::PnccdModule::create   (Cds& cds)
{
  _cds = &cds;
  
  for(std::list<ModulePlots*>::iterator it=_plots.begin();
      it!=_plots.end(); it++)
    (*it)->add(_cds);
}

void Ami::PnccdModule::analyze  () {}

const char* Ami::PnccdModule::name() const { return "PNCCD Analysis"; }

bool Ami::PnccdModule::accept () { return true; }


Ami::PnccdModule::ModulePlots::ModulePlots(const Pds::DetInfo& info,
					   void* payload) :
  _info  (info),
  _config(*reinterpret_cast<const Pds::PNCCD::ConfigV2*>(payload)),
  _common_dist(0)
{
  // create plots
  char buffer[64];
  { sprintf(buffer,"Pixel Values#%s#%d#%d",
	    _info.name(_info),0,0);
    DescImage idsc(buffer, 128*4, rows_segment, 16, 1);
    _dist = new Ami::EntryImage(idsc);
    _dist->info(0.,EntryImage::Pedestal);
    _dist->info(1.,EntryImage::Normalization); }

  static unsigned pcolor[] = { 0, 0xff, 0xff00, 0xff0000, 0xff00ff, 0x008080, 0x800080 };
  for(unsigned iq=0; iq<4; iq++) {
    { sprintf(buffer,"Energy#%s-Q%d#%d#%d#0x%x",
	      _info.name(_info),iq,0,0,pcolor[6]);
      DescTH1F edsc(buffer, "Energy", "Events", nrg_bins, nrg_min, nrg_min+nrg_range);
      _enrg   [iq] = new EntryTH1F(edsc); }
    { sprintf(buffer,"Energy(max)#%s-Q%d#%d#%d#0x%x",
	      _info.name(_info),iq,0,0,pcolor[5]);
      DescTH1F edsc(buffer, "Energy", "Events", nrg_bins, nrg_min, nrg_min+nrg_range);
      _enrg_ctr[iq] = new EntryTH1F(edsc); }
    { sprintf(buffer,"Energy(sum)#%s-Q%d#%d#%d#0x%x",
	      _info.name(_info),iq,0,0,pcolor[4]);
      DescTH1F edsc(buffer, "Energy", "Events", nrg_bins, nrg_min, nrg_min+nrg_range);
      _enrg_sum[iq] = new EntryTH1F(edsc); }
    for(unsigned j=0; j<4; j++) {
      { sprintf(buffer,"Energy(sum) vm[%d]#%s-Q%d-vm#%d#%d#0x%x",
		0x3fc0+j*0x10,_info.name(_info),iq,0,0,pcolor[j]);
	DescTH1F edsc(buffer, "Energy", "Events", 400, 6000, 12000);
	_enrg_sum_vm[iq*16+j+0] = new EntryTH1F(edsc); }
      { sprintf(buffer,"Energy(sum) vm[%d]#%s-Q%d-vm#%d#%d#0x%x",
		0x3e00+j*0x80,_info.name(_info),iq,0,1,pcolor[j]);
	DescTH1F edsc(buffer, "Energy", "Events", 400, 6000, 12000);
	_enrg_sum_vm[iq*16+j+4] = new EntryTH1F(edsc); }
      { sprintf(buffer,"Energy(sum) vm[%d]#%s-Q%d-vm#%d#%d#0x%x",
		0x3800+j*0x200,_info.name(_info),iq,1,0,pcolor[j]);
	DescTH1F edsc(buffer, "Energy", "Events", 400, 6000, 12000);
	_enrg_sum_vm[iq*16+j+8] = new EntryTH1F(edsc); }
      { sprintf(buffer,"Energy(sum) vm[%d]#%s-Q%d-vm#%d#%d#0x%x",
		0x2000+j*0x800,_info.name(_info),iq,1,1,pcolor[j]);
	DescTH1F edsc(buffer, "Energy", "Events", 400, 6000, 12000);
	_enrg_sum_vm[iq*16+j+12] = new EntryTH1F(edsc); }
    }
    { sprintf(buffer,"Energy(1p)#%s-Q%d#%d#%d#0x%x",
	      _info.name(_info),iq,0,0,pcolor[0]);
      DescTH1F edsc(buffer, "Energy", "Events", nrg_bins, nrg_min, nrg_min+nrg_range);
      _enrg_1p[iq] = new EntryTH1F(edsc); }
    { sprintf(buffer,"Energy(2p)#%s-Q%d#%d#%d#0x%x",
	      _info.name(_info),iq,0,0,pcolor[1]);
      DescTH1F edsc(buffer, "Energy", "Events", nrg_bins, nrg_min, nrg_min+nrg_range);
      _enrg_2p[iq] = new EntryTH1F(edsc); }
    { sprintf(buffer,"Energy(3p)#%s-Q%d#%d#%d#0x%x",
	      _info.name(_info),iq,0,0,pcolor[2]);
      DescTH1F edsc(buffer, "Energy", "Events", nrg_bins, nrg_min, nrg_min+nrg_range);
      _enrg_3p[iq] = new EntryTH1F(edsc); 
    { sprintf(buffer,"Energy(4p)#%s-Q%d#%d#%d#0x%x",
	      _info.name(_info),iq,0,0,pcolor[3]);
      DescTH1F edsc(buffer, "Energy", "Events", nrg_bins, nrg_min, nrg_min+nrg_range);
      _enrg_4p[iq] = new EntryTH1F(edsc); }
    }
    { sprintf(buffer,"Combinatoric#%s-Q%d#%d#%d",
	      _info.name(_info),iq,0,1);
      DescTH1F edsc(buffer, "Combinatoric", "Events", 256, -0.5, 255.5);
      _comb[iq] = new EntryTH1F(edsc); }
    { sprintf(buffer,"Ratio#%s-Q%d#%d#%d",
	      _info.name(_info),iq,1,1);
      DescProf edsc(buffer, "Neighbor", "Ratio", 8, 0, 8, 0);
      _erat   [iq] = new EntryProf(edsc); }
    { sprintf(buffer,"Ratio(2p)#%s-Q%d#%d#%d#0x%x",
	      _info.name(_info),iq,1,0,pcolor[1]);
      DescTH1F edsc(buffer, "Energy", "Events", 100, 0., 0.5);
      _erat_2p[iq] = new EntryTH1F(edsc); }
    { sprintf(buffer,"Ratio(3p)#%s-Q%d#%d#%d#0x%x",
	      _info.name(_info),iq,1,0,pcolor[2]);
      DescTH1F edsc(buffer, "Energy", "Events", 100, 0., 0.5);
      _erat_3p[iq] = new EntryTH1F(edsc); 
    { sprintf(buffer,"Ratio(4p)#%s-Q%d#%d#%d#0x%x",
	      _info.name(_info),iq,1,0,pcolor[3]);
      DescTH1F edsc(buffer, "Energy", "Events", 100, 0., 0.5);
      _erat_4p[iq] = new EntryTH1F(edsc); }
    }
  }

  reset();
}

Ami::PnccdModule::ModulePlots::~ModulePlots()
{
  delete _dist;
  for(unsigned iq=0; iq<4; iq++) {
    delete _enrg   [iq];
    delete _enrg_ctr[iq];
    delete _enrg_sum[iq];
    for(unsigned j=0; j<16; j++)
      delete _enrg_sum_vm[iq*16+j];
    delete _enrg_1p[iq];
    delete _enrg_2p[iq];
    delete _enrg_3p[iq];
    delete _enrg_4p[iq];
    delete _comb   [iq];
    delete _erat   [iq];
    delete _erat_2p[iq];
    delete _erat_3p[iq];
    delete _erat_4p[iq];
  }
  if (_common_dist)
    delete[] _common_dist;
}

void Ami::PnccdModule::ModulePlots::analyze(void* payload)
{
  const Pds::PNCCD::FramesV1& f = *reinterpret_cast<const Pds::PNCCD::FramesV1*>(payload);
  _fillQuadrant (f, 0);
  _fillQuadrant (f, 1);
  _fillQuadrant (f, 2);
  _fillQuadrant (f, 3);
  _dist->valid(__clk);
  _n++;
}

void Ami::PnccdModule::ModulePlots::_fillQuadrant (const  Pds::PNCCD::FramesV1& f,
						   unsigned iq)
{
  //  The calibrated frame contents
  ndarray<int32_t,2> w = make_ndarray<int32_t>(rows_segment,cols_segment);

  //
  //  Estimate common mode and noise threshold
  //
  int sigma_sum =0;
  int nsigma_sum=0;

  const uint16_t* d = f.frame(_config,iq).data(_config).data();
  
  const int nx  = _dist->desc().nbinsx()/8;
  const int ppx = _dist->desc().ppxbin();
  
  for(unsigned j=0; j<rows_segment; j++,d+=cols_segment) {

    //
    //  Find the common mode correction
    //
    uint32_t* p = &_dist->content()[j][iq*2*nx];

    memset(p,0,nx*sizeof(uint32_t));

    int32_t* wp = &w[j][0];
    for(unsigned k=0; k<cols_segment; k++) {
      int v = (d[k] & 0x3fff);
      if (v==0x3fff) {
	v -= _ped[iq][j][k];
	wp[k] = -200;
      }
      else
	wp[k] = (v -= _ped[iq][j][k]);
      v = v/ppx + nx/2;
      if (v < 0)
	p[0]++;
      else if (v >= nx)
	p[nx-1]++;
      else
	p[v]++;
#ifdef DBUG
      if (j==0 && k==0)
	printf("d %d  ped %d  w %d\n",d[0],_ped[iq][0][0],wp[0]);
#endif
    }

    int common_mode =
      int(FrameCalib::median(make_ndarray<int32_t>(wp,cols_segment),_common_lo,_common_hi,_common_dist));

    //
    //  Apply the common mode correction
    //
    p = &_dist->content()[j][(iq*2+1)*nx];

    memset(p,0,nx*sizeof(uint32_t));

    for(unsigned k=0; k<cols_segment; k++) {
      int v = (wp[k]-=common_mode);
      v = v/ppx + nx/2;
      if (v < 0)
	p[0]++;
      else if (v >= nx)
	p[nx-1]++;
      else
	p[v]++;
    }

#if 0
    if (_n < ped_reject)
      _ped_calib[iq].init (j,wp);
    else if (_n < ped_events)
      _ped_calib[iq].accum(j,wp);
#endif

    //  estimate one sigma
    int r=cols_segment/6;
    int i = 0;
    while( r>0 ) 
      r-=p[i++];
    
    if (i>1) {
      sigma_sum += ppx*(nx/2-i);
      nsigma_sum++;
    }
  }

#if 0
  { 
    const PedCalib& calib = _ped_calib[iq];
    if (_n < ped_events)
      return;
    else if (_n == ped_events) {
      int n = _n-ped_reject;
      for(unsigned iy=0; iy<rows_segment; iy++)
	for(unsigned ix=0; ix<cols_segment; ix++)
	  _ped[iq][iy][ix] += (calib.accumulated(ix,iy)+n/2)/n;
      //  perhaps common mode distribution will get narrower
      _common_lo = -255;
      _common_hi =  256;
      return;
    }
  }
#endif

  const double nrg_cut = 1000;
  const double nbr_cut = nsigma_sum ? double(3*sigma_sum) / double(nsigma_sum) : (1<<14);
#ifdef DBUG
  printf("  sigma sum %d  n %d  cut %f\n",
	 sigma_sum, nsigma_sum, nrg_cut);
#endif

  //
  //  Find droplets
  //
  _analyze_droplets(w,f.frame(_config,iq).data(_config),
		    iq,nrg_cut,nbr_cut);
}

void Ami::PnccdModule::ModulePlots::remove(Cds* _cds)
{
  _cds->remove(_dist);
  for(unsigned iq=0; iq<4; iq++) {
    _cds->remove(_enrg  [iq]);
    _cds->remove(_enrg_ctr[iq]);
    _cds->remove(_enrg_sum[iq]);
    for(unsigned j=0; j<16; j++)
      _cds->remove(_enrg_sum_vm[iq*16+j]);
    _cds->remove(_enrg_1p[iq]);
    _cds->remove(_enrg_2p[iq]);
    _cds->remove(_enrg_3p[iq]);
    _cds->remove(_enrg_4p[iq]);
    _cds->remove(_comb   [iq]);
    _cds->remove(_erat   [iq]);
    _cds->remove(_erat_2p[iq]);
    _cds->remove(_erat_3p[iq]);
    _cds->remove(_erat_4p[iq]);
  }
}

void Ami::PnccdModule::ModulePlots::reset()
{
  // lookup pedestal
  DescImage pdsc(_info,unsigned(0),"peds",cols,rows);
  EntryImage* correct = new EntryImage(pdsc);
  PnccdCalib::load_pedestals(correct,false);
  ndarray<const unsigned,2> ped = correct->content();

  for(unsigned i=0; i<4; i++)
    _ped[i] = make_ndarray<uint16_t>(rows,cols);
  for(unsigned i=0; i<rows_segment; i++)
    for(unsigned j=0; j<cols_segment; j++) {
      _ped[0][i][j] = ped[i][j];
      _ped[1][i][j] = ped[rows-1-i][cols_segment-1-j];
      _ped[2][i][j] = ped[rows-1-i][cols-1-j];
      _ped[3][i][j] = ped[i][j+cols_segment];
    }
  delete correct;

  for(unsigned i=0; i<4; i++)
    _ped_calib[i].reset();

 _dist->reset();
  for(unsigned iq=0; iq<4; iq++) {
    _enrg   [iq]->reset();
    _enrg_ctr[iq]->reset();
    _enrg_sum[iq]->reset();
    for(unsigned j=0; j<16; j++)
      _enrg_sum_vm[iq*16+j]->reset();
    _enrg_1p[iq]->reset();
    _enrg_2p[iq]->reset();
    _enrg_3p[iq]->reset();
    _enrg_4p[iq]->reset();
    _comb   [iq]->reset();
    _erat   [iq]->reset();
    _erat_2p[iq]->reset();
    _erat_3p[iq]->reset();
    _erat_4p[iq]->reset();
  }

  _n = 0;
  _common_lo = -255;
  _common_hi =  256;
  if (_common_dist)
    delete[] _common_dist;
  _common_dist = new unsigned[_common_hi-_common_lo+1];
}

void Ami::PnccdModule::ModulePlots::add(Cds* _cds)
{
  _cds->add(_dist);
  for(unsigned iq=0; iq<4; iq++) {
    _cds->add(_enrg   [iq]);
    _cds->add(_enrg_ctr[iq]);
    _cds->add(_enrg_sum[iq]);
    for(unsigned j=0; j<16; j++)
      _cds->add(_enrg_sum_vm[iq*16+j]);
    _cds->add(_enrg_1p[iq]);
    _cds->add(_enrg_2p[iq]);
    _cds->add(_enrg_3p[iq]);
    _cds->add(_enrg_4p[iq]);
    _cds->add(_comb   [iq]);
    _cds->add(_erat   [iq]);
    _cds->add(_erat_2p[iq]);
    _cds->add(_erat_3p[iq]);
    _cds->add(_erat_4p[iq]);
  }
}

void Ami::PnccdModule::ModulePlots::_analyze_droplets(ndarray<int32_t,2> v,
						      ndarray<const uint16_t,2> d,
						      unsigned           iq,
						      double             nrg_cut,
						      double             nbr_cut)
{
  for(unsigned iy=1; iy<v.shape()[0]-1; iy++) {
    const int32_t* uv = &v[iy-1][1];
    const int32_t* cv = &v[iy+0][1];
    const int32_t* dv = &v[iy+1][1];
    for(unsigned ix=1; ix<v.shape()[1]-1; ix++, uv++, cv++, dv++) {
      int z = cv[0];
      _enrg[iq]->addcontent(1.,double(z));

      if (z > nrg_cut  && z > cv[-1] && z > cv[+1] &&
	  z > uv[0]    && z > uv[-1] && z > uv[+1] &&
	  z > dv[0]    && z > dv[-1] && z > dv[+1] &&
	  z < nrg_max) {  // local maximum

	_enrg_ctr[iq]->addcontent(1.,double(z));

	//  correct for ADC cable distortions
	const double RQ = -0.06/(0.94*0.94);
	const double DQ = 1./0.94;
	double uvr, uvc, uvl;
	double cvr, cvc, cvl;
	double dvr, dvc, dvl;
	if (ix==127) {
	  uvr = DQ*double(uv[ 1]);
	  cvr = DQ*double(cv[ 1]);
	  dvr = DQ*double(dv[ 1]);
	  uvc = DQ*double(uv[ 0])+RQ*double(uv[-1]);
	  cvc = DQ*double(cv[ 0])+RQ*double(cv[-1]);
	  dvc = DQ*double(dv[ 0])+RQ*double(dv[-1]);
	  uvl = DQ*double(uv[-1]);
	  cvl = DQ*double(cv[-1]);
	  dvl = DQ*double(dv[-1]);
	}
	else if (ix==128) {
	  uvr = DQ*double(uv[ 1])+RQ*double(uv[ 0]);
	  cvr = DQ*double(cv[ 1])+RQ*double(cv[ 0]);
	  dvr = DQ*double(dv[ 1])+RQ*double(dv[ 0]);
	  uvc = DQ*double(uv[ 0]);
	  cvc = DQ*double(cv[ 0]);
	  dvc = DQ*double(dv[ 0]);
	  uvl = DQ*double(uv[-1]);
	  cvl = DQ*double(cv[-1]);
	  dvl = DQ*double(dv[-1]);
	}
	else {
	  uvr = DQ*double(uv[ 1])+RQ*double(uv[ 0]);
	  cvr = DQ*double(cv[ 1])+RQ*double(cv[ 0]);
	  dvr = DQ*double(dv[ 1])+RQ*double(dv[ 0]);
	  uvc = DQ*double(uv[ 0])+RQ*double(uv[-1]);
	  cvc = DQ*double(cv[ 0])+RQ*double(cv[-1]);
	  dvc = DQ*double(dv[ 0])+RQ*double(dv[-1]);
	  uvl = DQ*double(uv[-1]);
	  cvl = DQ*double(cv[-1]);
	  dvl = DQ*double(dv[-1]);
	}

	double dz = cvc;
	unsigned mask=0;
	if (uvl > nbr_cut) { mask|=1<<0; dz+=uvl; }
	if (uvc > nbr_cut) { mask|=1<<1; dz+=uvc; }
	if (uvr > nbr_cut) { mask|=1<<2; dz+=uvr; }
	if (cvl > nbr_cut) { mask|=1<<3; dz+=cvl; }
	if (cvr > nbr_cut) { mask|=1<<4; dz+=cvr; }
	if (dvl > nbr_cut) { mask|=1<<5; dz+=dvl; }
	if (dvc > nbr_cut) { mask|=1<<6; dz+=dvc; }
	if (dvr > nbr_cut) { mask|=1<<7; dz+=dvr; }

	_erat[iq]->addy(uvl/cvc,0U);
	_erat[iq]->addy(uvc/cvc,1U);
	_erat[iq]->addy(uvr/cvc,2U);
	_erat[iq]->addy(cvl/cvc,3U);
	_erat[iq]->addy(cvr/cvc,4U);
	_erat[iq]->addy(dvl/cvc,5U);
	_erat[iq]->addy(dvc/cvc,6U);
	_erat[iq]->addy(dvr/cvc,7U);

	_enrg_sum[iq]->addcontent(1.,dz);

	int dxy = int(d[iy][ix]&0x3fff);
	{ int dm = dxy - 0x3fc0;
	  if (dm > 0)
	    _enrg_sum_vm[iq*16+dm/0x10]->addcontent(1.,dz); }
	{ int dm = dxy - 0x3e00;
	  if (dm > 0)
	    _enrg_sum_vm[iq*16+dm/0x80+4]->addcontent(1.,dz); }
	{ int dm = dxy - 0x3800;
	  if (dm > 0)
	    _enrg_sum_vm[iq*16+dm/0x200+8]->addcontent(1.,dz); }
	{ int dm = dxy - 0x2000;
	  if (dm > 0)
	    _enrg_sum_vm[iq*16+dm/0x800+12]->addcontent(1.,dz); }

	_comb[iq]->addcontent(1.,mask);

	switch(mask) {
	case 0: // center only
	  _enrg_1p[iq]->addcontent(1.,dz); break;
	case (1<<4):
	case (1<<1):
	case (1<<3):
	case (1<<6):
	  _enrg_2p[iq]->addcontent(1.,dz); 
	  _erat_2p[iq]->addcontent(1.,(dz-cvc)/dz);
	  break;
	case ((1<<1)|(1<<3)):
	  _erat_3p[iq]->addcontent(1.,uvc/dz);
	  _erat_3p[iq]->addcontent(1.,cvl/dz);
	  _enrg_3p[iq]->addcontent(1.,dz); 
	  break;
	case ((1<<1)|(1<<4)):
	  _erat_3p[iq]->addcontent(1.,uvc/dz);
	  _erat_3p[iq]->addcontent(1.,cvr/dz);
	  _enrg_3p[iq]->addcontent(1.,dz); 
	  break;
	case ((1<<6)|(1<<3)):
	  _erat_3p[iq]->addcontent(1.,dvc/dz);
	  _erat_3p[iq]->addcontent(1.,cvl/dz);
	  _enrg_3p[iq]->addcontent(1.,dz); 
	  break;
	case ((1<<6)|(1<<4)):
	  _erat_3p[iq]->addcontent(1.,dvc/dz);
	  _erat_3p[iq]->addcontent(1.,cvr/dz);
	  _enrg_3p[iq]->addcontent(1.,dz); 
	  break;
	case ((1<<1)|(1<<3)|(1<<0)):
	  _erat_4p[iq]->addcontent(1.,(uvc+uvl)/dz);
	  _erat_4p[iq]->addcontent(1.,(cvl+uvl)/dz);
	  _enrg_4p[iq]->addcontent(1.,dz); 
	  break;
	case ((1<<1)|(1<<4)|(1<<2)):
	  _erat_4p[iq]->addcontent(1.,(uvc+uvr)/dz);
	  _erat_4p[iq]->addcontent(1.,(cvr+uvr)/dz);
	  _enrg_4p[iq]->addcontent(1.,dz); 
	  break;
	case ((1<<6)|(1<<3)|(1<<5)):
	  _erat_4p[iq]->addcontent(1.,(cvl+dvl)/dz);
	  _erat_4p[iq]->addcontent(1.,(dvc+dvl)/dz);
	  _enrg_4p[iq]->addcontent(1.,dz); 
	  break;
	case ((1<<6)|(1<<4)|(1<<7)):
	  _erat_4p[iq]->addcontent(1.,(cvr+dvr)/dz);
	  _erat_4p[iq]->addcontent(1.,(dvc+dvr)/dz);
	  _enrg_4p[iq]->addcontent(1.,dz); 
	  break;
	default:
	  break;
	}
      }
      else {
	//	_enrg_1p[iq]->addcontent(1.,double(z));
      }
    }
  }
  _enrg   [iq]->valid(__clk);
  _enrg_ctr[iq]->valid(__clk);
  _enrg_sum[iq]->valid(__clk);
  for(unsigned j=0; j<16; j++)
    _enrg_sum_vm[iq*16+j]->valid(__clk);
  _enrg_1p[iq]->valid(__clk);
  _enrg_2p[iq]->valid(__clk);
  _enrg_3p[iq]->valid(__clk);
  _enrg_4p[iq]->valid(__clk);
  _comb   [iq]->valid(__clk);
  _erat  [iq]->valid(__clk);
  _erat_2p[iq]->valid(__clk);
  _erat_3p[iq]->valid(__clk);
  _erat_4p[iq]->valid(__clk);
}

//
//  Plug-in module creator
//

extern "C" Ami::UserModule* create() { return new Ami::PnccdModule; }

extern "C" void destroy(Ami::UserModule* p) { delete p; }
