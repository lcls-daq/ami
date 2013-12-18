#include "TimeToolM.hh"

#include "ami/data/Cds.hh"

#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryTH1F.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/compress/CompressedXtc.hh"
#include "pdsdata/psddl/epics.ddl.h"

#include "pdsdata/psddl/camera.ddl.h"
#include "pdsdata/psddl/opal1k.ddl.h"
#include "pdsdata/psddl/evr.ddl.h"
#include "pdsdata/psddl/lusi.ddl.h"

using namespace Ami;

#include "pdsdata/psddl/encoder.ddl.h"

#include <math.h>
#include <stdlib.h>
#include <string>
#include <fstream>

using std::string;

typedef Pds::Opal1k::ConfigV1 Opal1kConfig;
typedef Pds::EvrData::DataV3 EvrDataType;

#include "timetool/service/Fex.hh"
#include "timetool/service/RatioFit.hh"

static const int   cols  = Pds::Opal1k::ConfigV1::Column_Pixels;

//
//  Create all plot entries
//
static EntryTH1F* one_evt(DescTH1F d) {
  d.aggregate(false);
  return new EntryTH1F(d);
}


namespace Ami {

  class FexM : public TimeTool::Fex {
  public:
    FexM(const char* fname) : Fex(fname) {}
    ~FexM() {}
  public:
    void _monitor_raw_sig(const uint32_t* wf, double* sb) 
    {
      for(int k=_spec_begin; k<_spec_end; k++)
        _raw_signal->content(double(wf[k])-sb[k],k);
    }
    void _monitor_ref_sig(const double* wf) 
    {
      for(int k=_spec_begin; k<_spec_end; k++)
        _ref_signal->content(wf[k],k);
    }
    void _monitor_sub_sig (const double* wf)
    {
      for(int k=_spec_begin; k<_spec_end; k++)
        _sub_signal->content(wf[k],k);
    }
    void _monitor_sub_sig_u (const double* wf)
    {
      for(int k=_spec_begin; k<_spec_end; k++)
        _sub_signal_u->content(wf[k],k);
    }
    void _monitor_flt_sig (const double* wf)
    {
      for(int k=_spec_begin+_nwts; k<_spec_end; k++)
        _flt_signal->content(wf[k],k);
    }
    void _monitorcorr    (const TimeTool::RatioFit& f0,
                           const TimeTool::RatioFit& f1)
    {
#if 0
      _p0corr->addy(f1.p0(),f0.p0());
      _p1corr->addy(f1.p1(),f0.p1());
#endif
    }
  public:
    int cache(Ami::FeatureCache& cache) {
      _cache = &cache;
      _cache_index = cache.add(base_name()+":AMI:AMPL"); 
      cache.add(base_name()+":AMI:FLTPOS"); 
      cache.add(base_name()+":AMI:FLTPOS_PS"); 
      cache.add(base_name()+":AMI:FLTPOSFWHM"); 
      cache.add(base_name()+":AMI:AMPLNXT"); 
      cache.add(base_name()+":AMI:REFAMPL"); 
      return _cache_index;
    }
    void configure() {
      TimeTool::Fex::configure();
      _frame = 0;
      _evrdata = 0;
      _ipmdata = 0;
      _sig_wf = 0;
      _sb_wf  = 0;
      _ref_wf = 0;
    }
    void configure(const Pds::Epics::EpicsPvCtrlHeader& pv) {
      const int slen = base_name().length();
      if (strncmp(pv.pvName(),base_name().c_str(),slen)==0) {
        if      (strcmp(&pv.pvName()[slen],":SIGNAL_WF")==0)
          _id_sig = pv.pvId();
        else if (strcmp(&pv.pvName()[slen],":SIDEBAND_WF")==0)
          _id_sb  = pv.pvId();
        else if (strcmp(&pv.pvName()[slen],":REFERENCE_WF")==0)
          _id_ref = pv.pvId();
      }
    }
    void event(const Pds::DetInfo&   src,
               const Pds::TypeId&    type,
               void*                 payload) {
      if (src.phy()==_phy) {
        if (type.id()==Pds::TypeId::Id_Frame)
          if (type.compressed()) {
            const Pds::Xtc* xtc = reinterpret_cast<const Pds::Xtc*>(payload)-1;
            _pXtc = Pds::CompressedXtc::uncompress(*xtc);
            _frame = reinterpret_cast<Pds::Camera::FrameV1*>(_pXtc->payload());
          }
          else
            _frame = reinterpret_cast<Pds::Camera::FrameV1*>(payload);
        else if (type.id()==Pds::TypeId::Id_Epics) {
          const Pds::Epics::EpicsPvTimeLong& pv = 
            *reinterpret_cast<const Pds::Epics::EpicsPvTimeLong*>(payload);
          if      (pv.pvId() == _id_sig) 
            _sig_wf = reinterpret_cast<const uint32_t*>(&pv+1);
          else if (pv.pvId() == _id_sb)
            _sb_wf  = reinterpret_cast<const uint32_t*>(&pv+1);
          else if (pv.pvId() == _id_ref)
            _ref_wf = reinterpret_cast<const uint32_t*>(&pv+1);
        }
      }
      else if (type.id()==Pds::TypeId::Id_EvrData) {
        _evrdata = reinterpret_cast<Pds::EvrData::DataV3*>(payload);
      }
      else if (src.level()==_ipm_no_beam_src.level() && 
               src.phy  ()==_ipm_no_beam_src.phy  () &&
               type.id  ()==Pds::TypeId::Id_IpmFex)
        _ipmdata = reinterpret_cast<Pds::Lusi::IpmFexV1*>(payload);
    }
    void clear(Cds* _cds) {
      if (_cds) {
        _cds->remove(_ref_signal);
        _cds->remove(_raw_signal);
        _cds->remove(_sub_signal);
        _cds->remove(_sub_signal_u);
        _cds->remove(_flt_signal);
        delete _ref_signal;
        delete _raw_signal;
        delete _sub_signal;
        delete _sub_signal_u;
        delete _flt_signal;
#if 0
        _cds->remove(_p0corr);
        _cds->remove(_p1corr);
        delete _p0corr;
        delete _p1corr;
#endif
      }
      TimeTool::Fex::configure();
    }
    void create(Cds& cds, unsigned column) {
      char buff[128];
      sprintf(buff,"Ref Signal#Signal#0#%d#0",column);
      _ref_signal  = one_evt(DescTH1F(buff,"ADU","Bin",cols,0.,double(cols)));
      sprintf(buff,"Raw Signal#Signal#0#%d#c0",column);
      _raw_signal  = one_evt(DescTH1F(buff,"ADU","Bin",cols,0.,double(cols)));
      sprintf(buff,"Sub Signal#Signal#1#%d#c0",column);
      _sub_signal  = one_evt(DescTH1F(buff,"ADU","Bin",cols,0.,double(cols)));
      sprintf(buff,"Sub Signal U#Signal#1#%d#c000",column);
      _sub_signal_u  = one_evt(DescTH1F(buff,"ADU","Bin",cols,0.,double(cols)));
      sprintf(buff,"Flt Signal#Signal#1#%d#c00000",column);
      _flt_signal  = one_evt(DescTH1F(buff,"ADU","Bin",cols,0.,double(cols)));
      cds.add(_ref_signal);
      cds.add(_raw_signal);
      cds.add(_sub_signal);
      cds.add(_sub_signal_u);
      cds.add(_flt_signal);
#if 0
      _p0corr      = new EntryScan(DescScan("P0#Reference","Reference","Signal",100));
      _p1corr      = new EntryScan(DescScan("P1#Reference","Reference","Signal",100));
      cds.add(_p0corr);
      cds.add(_p1corr);
#endif
    }
    void analyze(const Pds::ClockTime& _clk) {
      if (_evrdata) {

        bool bykik    = false;
        bool no_laser = false;
        unsigned laser_code = abs(_event_code_no_laser);
        unsigned bykik_code = abs(_event_code_bykik);
        for(unsigned i=0; i<_evrdata->numFifoEvents(); i++) {
          const Pds::EvrData::FIFOEvent& fe = _evrdata->fifoEvents()[i];
          if (fe.eventCode() == bykik_code)
            bykik = true;
          if (fe.eventCode() == laser_code)
            no_laser = true;
        }

        if (int(_event_code_no_laser) < 0)
          no_laser = !no_laser;
        if (int(_event_code_bykik   ) < 0)
          bykik = !bykik;

        if (_ipmdata) {
          if (_ipmdata->sum() < _ipm_no_beam_threshold)
            bykik = true;
        }

        bool lpass = write_image() && _frame;
        if (lpass)
          TimeTool::Fex::analyze(*_frame, bykik, no_laser);
        else if ((lpass = write_projections() && _sig_wf))
          TimeTool::Fex::analyze(_sig_wf, _sb_wf, _ref_wf, bykik, no_laser);
        else if ((lpass = _frame))
          TimeTool::Fex::analyze(*_frame, bykik, no_laser);

        if (lpass) {

#if 0
          printf("fex status %c  ampl %f  pos %f  ref_a %f  next_a %f\n",
                 fex.status() ? 't':'f',
                 fex.amplitude(),
                 fex.filtered_position(),
                 fex.ref_amplitude(),
                 fex.next_amplitude());
#endif

          if (_cache) {
            _cache->cache(_cache_index+0, amplitude());
            _cache->cache(_cache_index+1, filtered_position());
            _cache->cache(_cache_index+2, filtered_pos_ps());
            _cache->cache(_cache_index+3, filtered_fwhm());
            _cache->cache(_cache_index+4, next_amplitude());
            _cache->cache(_cache_index+5, ref_amplitude());
          }

          if (status()) {
            int ix = int(filtered_position()+_indicator_offset);
            if (ix>=0 && ix<cols)
              _sub_signal->content(1.,ix);
          }

          _ref_signal->valid(_clk);
          _raw_signal->valid(_clk);
          _sub_signal->valid(_clk);
          _sub_signal_u->valid(_clk);
          _flt_signal->valid(_clk);
#if 0
          _p0corr    ->valid(_clk);
          _p1corr    ->valid(_clk);
#endif
        }
      }

      //  Reset pointer references
      _frame = 0;
      _evrdata = 0;
      _ipmdata = 0;
      _sig_wf = 0;
      _sb_wf  = 0;
      _ref_wf = 0;
    }
  private:
    EntryTH1F* _ref_signal;
    EntryTH1F* _raw_signal;
    EntryTH1F* _sub_signal;
    EntryTH1F* _sub_signal_u;
    EntryTH1F* _flt_signal;
#if 0
    EntryScan* _p0corr;
    EntryScan* _p1corr;
#endif
    int _id_sig;
    int _id_sb;
    int _id_ref;
    Pds::Camera::FrameV1* _frame;
    Pds::EvrData::DataV3* _evrdata;
    Pds::Lusi::IpmFexV1*  _ipmdata;
    const uint32_t* _sig_wf;
    const uint32_t* _sb_wf;
    const uint32_t* _ref_wf;
    Ami::FeatureCache*   _cache;
    int                  _cache_index;
    boost::shared_ptr<Pds::Xtc> _pXtc;
  };
};


TimeToolM::TimeToolM() : 
  _cds(0),
  _cache    (0)
{
  const char* fname = "timetool.input";
  char buff[128];
  const char* dir = getenv("HOME");
  sprintf(buff,"%s/%s", dir ? dir : "/tmp", fname);
  std::ifstream f(buff);
  if (f) {
    while(!f.eof()) {
      string sline;
      std::getline(f,sline);
      if (sline[0]=='<')
        _fex.push_back(new FexM(sline.substr(1).c_str()));
    }
    if (_fex.size()==0)
      _fex.push_back(new FexM(fname));
  }
}

TimeToolM::~TimeToolM() 
{
  for(unsigned i=0; i<_fex.size(); i++)
    delete _fex[i];
}

void TimeToolM::reset(Ami::FeatureCache& cache)
{
  _cache = &cache;
  for(unsigned i=0; i<_fex.size(); i++) {
    _fex[i]->configure();
    if (_cds)
      _fex[i]->cache(cache);
  }
}

void TimeToolM::clock    (const Pds::ClockTime& clk) 
{
//   if (clk.seconds() > _clk.seconds() &&
//       (clk.seconds()&0xf)==0)
//     recreate();

  _clk=clk; 
}

//
//  Cache the configuration for camera
//
void TimeToolM::configure(const Pds::DetInfo&   src,
			  const Pds::TypeId&    type,
			  void*                 payload) 
{
  if (type.id()==Pds::TypeId::Id_Opal1kConfig) {
    for(unsigned i=0; i<_fex.size(); i++)
      if (_fex[i]->_phy == src.phy())
        _fex[i]->configure();
  }
  else if (type.id()==Pds::TypeId::Id_Epics) {
    for(unsigned i=0; i<_fex.size(); i++) {
      FexM& fex = *_fex[i];
      if (src.phy() == fex._phy)
        fex.configure(*reinterpret_cast<const Pds::Epics::EpicsPvCtrlHeader*>(payload));
    }
  }
}

//
//  Capture pointer to detector data we want
//
void TimeToolM::event    (const Pds::DetInfo&   src,
			  const Pds::TypeId&    type,
                          const Pds::Damage&    damage,
			  void*                 payload) 
{
  if (damage.value()) return;

  for(unsigned i=0; i<_fex.size(); i++)
    _fex[i]->event(src,type,payload);
}

//
//  Remove all plot entries
//
void TimeToolM::clear    () 
{
  for(unsigned i=0; i<_fex.size(); i++)
    _fex[i]->clear(_cds);
}

#if 0
static DescTH1F no_agg(DescTH1F d) { d.aggregate(false); return d; }
#endif

void TimeToolM::create   (Cds& cds)
{
  _cds = &cds; 
  for(unsigned i=0; i<_fex.size(); i++) {
    _fex[i]->create(cds,i);
    if (_cache)
      _fex[i]->cache(*_cache);
    _fex[i]->init_plots();
  }
}

//
//  Analyze data for current event from the pointers we captured
//
void TimeToolM::analyze  ()
{
  if (_cds)
    for(unsigned i=0; i<_fex.size(); i++)
      _fex[i]->analyze(_clk);
}


// FROM FILTER
const char* TimeToolM::name() const { return "TimeTool"; }

//
//  Analyze data for current event from the pointers we captured
//
bool TimeToolM::accept  () 
{
  analyze();
  return true;
}


//
//  Plug-in module creator
//

extern "C" UserModule* create() { return new TimeToolM; }

extern "C" void destroy(UserModule* p) { delete p; }
