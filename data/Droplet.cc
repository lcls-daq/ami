#include "Droplet.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/DescRef.hh"
#include "ami/data/EntryRef.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/ImageMask.hh"

#include "ami/data/Cds.hh"
#include "ami/data/XML.hh"
#include "ami/data/valgnd.hh"

#include "ami/event/Calib.hh"

#include "ndarray/ndarray.h"
#include "psalg/psalg.h"

#include <QtCore/QString>

#include <stdio.h>

//#define DBUG
//#define DBUGM

typedef std::list<ndarray<double,1> > PLIST;

static const unsigned STEP_SIZE = 50;

static PLIST find_droplets(const ndarray<const unsigned,2>&,
			   unsigned offset,
			   const Ami::DropletConfig&,
			   Ami::FeatureCache&, unsigned index,
			   const ndarray<const double,1> xcal,
			   const ndarray<const double,1> ycal);

static PLIST find_droplets(const ndarray<const unsigned,2>&,
			   const ndarray<const unsigned,1>& row_mask,
			   const ndarray<const unsigned,2>& mask,
			   unsigned offset,
			   const Ami::DropletConfig&,
			   Ami::FeatureCache&, unsigned index,
			   const ndarray<const double,1> xcal,
			   const ndarray<const double,1> ycal);


enum { Stage1, Stage2, Stage3, Stage4, Stage5, NumberOfCuts };
static const char* stage_name[] = { "DuplSeed",
				    "NpMax",
				    "EMax",
				    "NpMin",
				    "Emin",
				    NULL };

using namespace Ami;
using Ami::XML::QtPersistent;

static const char* _name[] = { "Esum", 
			       "Xpix",
			       "Ypix",
			       "Npix",
			       "Xmom",
			       "Ymom",
			       "R2mom",
			       "Ndrops" };

const char* Droplets::name(Droplets::Parameter p) { return p <= NumberOf ? _name[p] : "-Invalid-"; }


Droplet::Droplet(const char* name,
		 const DropletConfig& config) :
  AbsOperator       (AbsOperator::Droplet),
  _config           (config),
  _entry            (0)
{
  strncpy_val(_name, name, NAME_LEN);
}

Droplet::Droplet(const char*& p,
		 const DescEntry& input,
		 FeatureCache& cache) :
  AbsOperator       (AbsOperator::Droplet),
  _v                (true),
  _cache            (&cache)
{
  _extract(p, _name   , NAME_LEN);
  _extract(p, &_config, sizeof(_config));

  DescRef o(_name);
  _entry = new EntryRef(o);
  _entry->set(&_output);

#ifdef DBUG
  printf("Droplet %s config: seed %d  nbor %d  esum_min %d  max %d  npix_max %d\n",
	 _name,
	 _config.seed_threshold,
	 _config.nbor_threshold,
	 _config.esum_min,
	 _config.esum_max,
	 _config.npix_max);
#endif

  char buff[64];
  for(unsigned i=0; i<NumberOfCuts; i++) {
    sprintf(buff,"Droplet:%s:%s",_name,stage_name[i]);
    if (i==0)
      _index = cache.add(buff);
    else
      cache.add(buff);
  }

  //
  //  Load x,y sub-pixel position calibration
  //
  Calib::load_integral_symm(_xcal, input.info().phy(), "xmom", "X-moment calibration");
  Calib::load_integral_symm(_ycal, input.info().phy(), "ymom", "Y-moment calibration");
}

Droplet::~Droplet()
{
  if (_entry) delete _entry;
}

void Droplet::use() {}

const DescEntry& Droplet::_routput   () const 
{ 
  if (_entry) return _entry->desc();
  abort();
}

void*      Droplet::_serialize(void* p) const
{
  _insert(p, _name   , NAME_LEN);
  _insert(p, &_config, sizeof(_config));
  return p;
}

Entry&     Droplet::_operate(const Entry& e) const
{
  if (!e.valid())
    return *_entry;

  const EntryImage& entry = static_cast<const EntryImage&>(e);
  const DescImage& d = entry.desc();
  const ImageMask* mask = d.mask();

  Droplets& output = const_cast<Droplet*>(this)->_output;
  output.reset();

  const unsigned p   = unsigned(entry.info(EntryImage::Pedestal));

  PLIST photons;
  if (mask)
    photons = find_droplets(entry.content(), mask->row_mask(), mask->all_mask(), p,
			    _config, *_cache, _index, _xcal, _ycal);
				 
#if 0
  else if (d.nframes())
    for(unsigned fn=0; fn<d.nframes(); fn++)
      photons.splice(photons.end(),
		     find_droplets(entry.contents(fn)));
#endif
  else
    photons = find_droplets(entry.content(), p, 
			    _config, *_cache, _index, _xcal, _ycal);

  const unsigned n = photons.size();
#ifdef DBUG
  printf("Droplet::photons %d\n",n);
#endif
  if (n) {
    output.resize(n);
    PLIST::iterator it=photons.begin();
    for(unsigned i=0; i<n; i++,it++)
      output.append(it->data());
  }
  _entry->valid(e.time());
  return *_entry;
}

class DropletBuilder {
public:
  DropletBuilder(const ndarray<const unsigned,2> data,
		 const ndarray<const unsigned,2> mask,
		 unsigned                        offset,
		 const DropletConfig&            cfg,
		 const ndarray<const double,1>   xcal,
		 const ndarray<const double,1>   ycal);
  ~DropletBuilder();
public:
  bool              process(const unsigned*);
  ndarray<double,1> result () const;
  const unsigned*   ncut() const { return _ncut; }
private:
  bool _contains     (const unsigned*) const;
  void _add          (const unsigned*);
  bool _test_pixel   (const unsigned*);
  inline bool _add_neighbors(const unsigned*);
private:
  const unsigned*                 _seed;
  const ndarray<const unsigned,2> _data;
  const ndarray<const unsigned,2> _mask;
  unsigned                        _offset;
  const DropletConfig&            _cfg;
  unsigned                        _map_scale;
  unsigned*                       _map;
  double                          _results[Droplets::NumberOf];
  int                             _x;
  int                             _y;
  bool                            _valid;
  unsigned                        _ncut[NumberOfCuts];
  const ndarray<const double,1>   _xcal;
  const ndarray<const double,1>   _ycal;
};

PLIST find_droplets(const ndarray<const unsigned,2>& v,
		    unsigned offset,
		    const DropletConfig& c,
		    FeatureCache& cache,
		    unsigned index,
		    const ndarray<const double,1> xcal,
		    const ndarray<const double,1> ycal)
{
  PLIST result;

  DropletConfig cfg(c);
  cfg.seed_threshold += offset;
  cfg.nbor_threshold += offset;

#ifdef DBUG
  printf("find_droplets: v[2][2] %u  off %u  seed %u  nbor %u\n",
	 v[2][2],offset,cfg.seed_threshold,cfg.nbor_threshold);
#endif


  unsigned shape[] = {0,0};
  ndarray<unsigned,2> no_mask(shape);

  DropletBuilder builder(v, no_mask, offset, cfg, xcal, ycal);

  for(unsigned iy=1; iy<v.shape()[0]-1; iy++) {
    const unsigned* uv = &v(iy-1,1);
    const unsigned* cv = &v(iy+0,1);
    const unsigned* dv = &v(iy+1,1);
    for(unsigned ix=1; ix<v.shape()[1]-1; ix++, uv++, cv++, dv++) {
      unsigned z = cv[0];

      //
      //  Find local maximum above hit_threshold
      //    Only analyze when central pixel is greater than
      //    any we've already analyzed or >= to any we have
      //    yet to analyze
      //
      if (z > cfg.seed_threshold &&
	  z >  cv[-1] && z >= cv[+1] &&
	  z >  uv[0]  && z >  uv[-1] && z >  uv[+1] &&
	  z >= dv[0]  && z >= dv[-1] && z >= dv[+1])

	if (z < cfg.esum_max+offset)
	  if (builder.process(cv))
	    result.push_back(builder.result());
    }
  }

  for(unsigned i=0; i<NumberOfCuts; i++)
    cache.cache(index+i,builder.ncut()[i]);

  return result;
}

PLIST find_droplets(const ndarray<const unsigned,2>& v,
		    const ndarray<const unsigned,1>& row_mask,
		    const ndarray<const unsigned,2>& mask,
		    unsigned offset,
		    const DropletConfig& c,
		    FeatureCache& cache,
		    unsigned index,
		    const ndarray<const double,1> xcal,
		    const ndarray<const double,1> ycal)
{
  PLIST result;

  DropletConfig cfg(c);
  cfg.seed_threshold += offset;
  cfg.nbor_threshold += offset;

  //
  //  try removing the mask from the Droplet Builder
  //
  unsigned shape[] = {0,0};
  ndarray<unsigned,2> no_mask(shape);

  DropletBuilder builder(v, no_mask, offset, cfg, xcal, ycal);

#ifdef DBUGM
  printf("Mask shape [%d,%d]\n", mask.shape()[0], mask.shape()[1]);
#endif

  for(unsigned iy=1; iy<v.shape()[0]-1; iy++) {

    if (0==(row_mask[iy>>5]&(1<<(iy&0x1f))))
      continue;

#ifdef DBUGM
    printf("Row %d masked\n",iy);
#endif

    const unsigned* uv = &v(iy-1,1);
    const unsigned* cv = &v(iy+0,1);
    const unsigned* dv = &v(iy+1,1);
    const unsigned* m = &mask(iy,0);
    for(unsigned ix=1; ix<v.shape()[1]-1; ix++, uv++, cv++, dv++) {

      if (0 == (m[ix>>5] & (1<<(ix&0x1f))))
	continue;

#ifdef DBUGM
      printf("[%d,%d] masked\n",ix,iy);
#endif

      unsigned z = cv[0];

      //
      //  Find local maximum above hit_threshold
      //    Only analyze when central pixel is greater than
      //    any we've already analyzed or >= to any we have
      //    yet to analyze
      //
      if (z > cfg.seed_threshold &&
	  z >  cv[-1] && z >= cv[+1] &&
	  z >  uv[0]  && z >  uv[-1] && z >  uv[+1] &&
	  z >= dv[0]  && z >= dv[-1] && z >= dv[+1])

	if (z < cfg.esum_max+offset)
	  if (builder.process(cv))
	    result.push_back(builder.result());
    }
  }

  for(unsigned i=0; i<NumberOfCuts; i++)
    cache.cache(index+i,builder.ncut()[i]);

  return result;
}

DropletBuilder::DropletBuilder(const ndarray<const unsigned,2> data,
			       const ndarray<const unsigned,2> mask,
			       unsigned                        offset,
			       const DropletConfig&            cfg,
			       const ndarray<const double,1>   xcal,
			       const ndarray<const double,1>   ycal) :
  _seed   (0),
  _data   (data),
  _mask   (mask),
  _offset (offset),
  _cfg    (cfg),
  _xcal   (xcal),
  _ycal   (ycal)
{
  _map_scale = 32;
  unsigned mlen  = _map_scale;
  unsigned msize = mlen*((mlen+31)/32);
  _map = new unsigned[msize];

  memset(&_ncut[0],0,NumberOfCuts*sizeof(unsigned));
}

DropletBuilder::~DropletBuilder()
{
  delete[] _map;
}

ndarray<double,1> DropletBuilder::result() const
{
  ndarray<double,1> a = make_ndarray<double>(Droplets::NumberOf);
  memcpy(a.data(),_results,Droplets::NumberOf*sizeof(double));
  return a;
}

bool DropletBuilder::process(const unsigned* seed)
{
  _seed = seed;

  unsigned nx = _data.shape()[1];
  unsigned i = seed-_data.data();
  _x = i%nx;
  _y = i/nx;

  int mlen  = _map_scale;
  unsigned msize = mlen*((mlen+31)/32);
  if (_mask.size()==0)
    memset(_map,0,msize*sizeof(unsigned));
  else {
    int y0 = _y>mlen/2 ? _y-mlen/2 : 0;
    int y1 = _y+mlen<int(_data.shape()[0]) ? mlen : _data.shape()[0]-_y;
    int iy = mlen/2+y0-_y;
    unsigned b = _x>>5;
    for(int y=y0; y<y1; y++,iy++)
      if (_x+32 > int(_data.shape()[1]))
	_map[iy] = ~((_mask(y,b)>>(_x&0x1f)));
      else
	_map[iy] = ~((_mask(y,b)>>(_x&0x1f)) |
		     (_mask(y,b+1)<<(32-(_x&0x1f))));
  }

  _add(seed);

  _results[Droplets::Esum] = double(*seed-_offset);
  _results[Droplets::Np  ] = 1;
  _results[Droplets::Xmom] = 0;
  _results[Droplets::Ymom] = 0;
  _results[Droplets::R2mom]= 0;

  if (!_add_neighbors(seed)) {
    return false;
  }

  if (_results[Droplets::Np] < _cfg.npix_min) {
    _ncut[Stage4]++;
    return false;
  }

  double e = _results[Droplets::Esum];
  if (e < _cfg.esum_min) {
    _ncut[Stage5]++;
    return false;
  }

  _results[Droplets::Xmom] /= e;
  _results[Droplets::Ymom] /= e;
  _results[Droplets::R2mom]/= e;

  _results[Droplets::X   ] = double(_x);
  _results[Droplets::Y   ] = double(_y);

  if (_xcal.size()) {
    double xm = _results[Droplets::Xmom];
    int ib = int((xm+0.5)*double(_xcal.size()));
    if (ib<0) ib=0;
    else if (ib>=int(_xcal.size())) ib=_xcal.size()-1;
    _results[Droplets::X] += _xcal[ib]-0.5;
  }

  if (_ycal.size()) {
    double ym = _results[Droplets::Ymom];
    int ib = int((ym+0.5)*double(_ycal.size()));
    if (ib<0) ib=0;
    else if (ib>=int(_ycal.size())) ib=_ycal.size()-1;
    _results[Droplets::Y] += _ycal[ib]-0.5;
  }

  return true;
}

bool DropletBuilder::_contains(const unsigned* p) const
{
  const unsigned nx = _data.shape()[1];
  unsigned i = p-_data.data();
  int dX = _map_scale/2+(i%nx)-_x;
  int dY = _map_scale/2+(i/nx)-_y;
  if (dX < 0 || dX >= int(_map_scale) ||
      dY < 0 || dY >= int(_map_scale))
    return true;

  unsigned b = dY*_map_scale + dX;
  return _map[b>>5] & (1<<(b&0x1f));
}

void DropletBuilder::_add(const unsigned* p)
{
  const unsigned nx = _data.shape()[1];
  unsigned i = p-_data.data();
  int dX = _map_scale/2+(i%nx)-_x;
  int dY = _map_scale/2+(i/nx)-_y;

  unsigned b = dY*_map_scale + dX;
  _map[b>>5] |= (1<<(b&0x1f));
}

//
//  Return value must be true for a valid droplet
//
bool DropletBuilder::_add_neighbors(const unsigned*  lseed)
{
  const unsigned nx = _data.shape()[1];
  if (!_test_pixel(lseed-1-nx)) return false;
  if (!_test_pixel(lseed+0-nx)) return false;
  if (!_test_pixel(lseed+1-nx)) return false;
  if (!_test_pixel(lseed-1   )) return false;
  if (!_test_pixel(lseed+1   )) return false;
  if (!_test_pixel(lseed-1+nx)) return false;
  if (!_test_pixel(lseed+0+nx)) return false;
  if (!_test_pixel(lseed+1+nx)) return false;
  return true;
}

//
//  Return value must be true for a valid droplet
//
bool DropletBuilder::_test_pixel(const unsigned* p)
{
  const unsigned nx = _data.shape()[1];

  if (p <  _data.data() ||
      p >= _data.end ()) return true;

  unsigned v = *p;
  if (v < _cfg.nbor_threshold) return true;

  //
  //  Has this pixel already been used in another droplet
  //
  if (v > _cfg.seed_threshold &&
      p < _seed) {
    const unsigned z = *p;
    const unsigned* cv = p;
    const unsigned* uv = p - nx;
    const unsigned* dv = p + nx;
    if ((uv-1) >= _data.data() &&
	(dv+1) <  _data.end () &&
	z >  cv[-1] && z >= cv[+1] &&
	z >  uv[0]  && z >  uv[-1] && z >  uv[+1] &&
	z >= dv[0]  && z >= dv[-1] && z >= dv[+1]) {
      _ncut[Stage1]++;
      return false;
    }
  }
  
  if (_contains(p)) return true;

  if (++_results[Droplets::Np  ] > _cfg.npix_max) {
    _ncut[Stage2]++;
    return false;
  }
  
  _add(p);

  double dE = double(*p-_offset);
  if ((_results[Droplets::Esum] += dE) > _cfg.esum_max) {
    _ncut[Stage3]++;
    return false;
  }
  
  unsigned i = p-_data.data();
  int dX = (i%nx)-_x;
  _results[Droplets::Xmom] += dE*double(dX);
  
  int dY = (i/nx)-_y;
  _results[Droplets::Ymom] += dE*double(dY);
  
  int dR2 = dX*dX+dY*dY;
  _results[Droplets::R2mom] += dE*double(dR2);
  
  if (!_add_neighbors(p))
    return false;

  return true;
}

DropletConfig::DropletConfig() :
  seed_threshold(0),
  nbor_threshold(0),
  esum_min      (0),
  esum_max      (0),
  npix_min      (1),
  npix_max      (1)
{
}

DropletConfig::DropletConfig(const DropletConfig& c)
{
  if (c.seed_threshold > c.nbor_threshold) {
    seed_threshold = c.seed_threshold;
    nbor_threshold = c.nbor_threshold;
  }
  else {
    seed_threshold = c.nbor_threshold;
    nbor_threshold = c.seed_threshold;
  }
  if (c.esum_min < c.esum_max) {
    esum_min = c.esum_min;
    esum_max = c.esum_max;
  }
  else {
    esum_min = c.esum_max;
    esum_max = c.esum_min;
  }
  if (c.npix_min < c.npix_max) {
    npix_min = c.npix_min;
    npix_max = c.npix_max;
  }
  else {
    npix_min = c.npix_max;
    npix_max = c.npix_min;
  }
}

void DropletConfig::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if      (tag.name == "seed_threshold") 
      seed_threshold = QtPersistent::extract_i(p);
    else if (tag.name == "nbor_threshold") 
      nbor_threshold = QtPersistent::extract_i(p);
    else if (tag.name == "esum_min") 
      esum_min = QtPersistent::extract_i(p);
    else if (tag.name == "esum_max") 
      esum_max = QtPersistent::extract_i(p);
    else if (tag.name == "npix_min") 
      npix_min = QtPersistent::extract_i(p);
    else if (tag.name == "npix_max") 
      npix_max = QtPersistent::extract_i(p);
  XML_iterate_close(DropletConfig,tag);
}

void DropletConfig::save(char*& p) const
{
  XML_insert(p, "unsigned", "seed_threshold", QtPersistent::insert(p,seed_threshold));
  XML_insert(p, "unsigned", "nbor_threshold", QtPersistent::insert(p,nbor_threshold));
  XML_insert(p, "unsigned", "esum_min", QtPersistent::insert(p,esum_min));
  XML_insert(p, "unsigned", "esum_max", QtPersistent::insert(p,esum_max));
  XML_insert(p, "unsigned", "npix_min", QtPersistent::insert(p,npix_min));
  XML_insert(p, "unsigned", "npix_max", QtPersistent::insert(p,npix_max));
}

void Droplet::_invalid() { _entry->invalid(); }
