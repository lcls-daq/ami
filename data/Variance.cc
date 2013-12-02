#include "Variance.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/SelfExpression.hh"
#include "ami/data/valgnd.hh"

#include "pdsalg/pdsalg.h"

#include <QtCore/QString>

#include <stdio.h>

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace Ami;

static void _zero(ndarray<double,2>& m1,
                  ndarray<double,2>& m2)
{
  const unsigned ny = m1.shape()[0];
  const unsigned nx = m1.shape()[1];
  for(unsigned iy=0; iy<ny; iy++)
    for(unsigned ix=0; ix<nx; ix++) {
      m1[iy][ix] = 0;
      m2[iy][ix] = 0;
    }
}

Variance::Variance(unsigned n, const char* scale) : 
  AbsOperator(AbsOperator::Variance),
  _n         (n),
  _i         (0),
  _cache     (0),
  _input     (0),
  _term      (0),
  _v         (true)
{
  strncpy_val(_scale_buffer,scale,SCALE_LEN);
}

Variance::Variance(const char*& p, const DescEntry& e, FeatureCache& features) :
  AbsOperator(AbsOperator::Variance),
  _i         (0),
  _input     (0),
  _v         (true)
{
  _extract(p,_scale_buffer, SCALE_LEN);
  _extract(p, &_n, sizeof(_n));

  _cache = EntryFactory::entry(e);
  _cache->reset();
  _cache->invalid();

  switch(e.type()) {
  case DescEntry::TH1F:
    { const DescTH1F& d = static_cast<const DescTH1F&>(e);
      _m1.push_back(make_ndarray<double>(1,d.nbins()));
      _m2.push_back(make_ndarray<double>(1,d.nbins()));
    } break;
  case DescEntry::Waveform:
    { const DescWaveform& d = static_cast<const DescWaveform&>(e);
      _m1.push_back(make_ndarray<double>(1,d.nbins()));
      _m2.push_back(make_ndarray<double>(1,d.nbins()));
    } break;
  case DescEntry::Image:
    { const DescImage& d = static_cast<const DescImage&>(e);
      if (d.nframes()) {
        for(unsigned k=0; k<d.nframes(); k++) {
          const SubFrame& f = d.frame(k);
          _m1.push_back(make_ndarray<double>(f.ny,f.nx));
          _m2.push_back(make_ndarray<double>(f.ny,f.nx));
        }
      }
      else {
        _m1.push_back(make_ndarray<double>(d.nbinsy(),d.nbinsx()));
        _m2.push_back(make_ndarray<double>(d.nbinsy(),d.nbinsx()));
      }
    } break;
  default:
    break;
  }

  for(unsigned k=0; k<_m1.size(); k++)
    _zero(_m1[k],_m2[k]);

  if (_scale_buffer[0]) {
    QString expr(_scale_buffer);
    SelfExpression parser;
    _term = parser.evaluate(features,expr,_input,e);
    if (!_term) {
      printf("BinMath failed to parse f %s\n",qPrintable(expr));
      _v = false; 
    }
  }
  else
    _term = 0;
}

Variance::~Variance()
{
  if (_cache) delete _cache;
  if (_term ) delete _term ;
}

void Variance::use() { if (_term) _term->use(); }

DescEntry& Variance::_routput   () const { return _cache->desc(); }

void*      Variance::_serialize(void* p) const
{
  _insert(p, _scale_buffer , SCALE_LEN);
  _insert(p, &_n, sizeof(_n));
  return p;
}

Entry&     Variance::_operate(const Entry& e) const
{
  if (e.valid()) {

    _input = &e;
    const double vn = _term ? _term->evaluate() : 1;
    std::vector<ndarray<const double,2> > in;
    ++_i;

    switch(e.desc().type()) {
    case DescEntry::TH1F    : 
      { const EntryTH1F& en = static_cast<const EntryTH1F&>(e);
        unsigned shape[] = {1,en.desc().nbins()};
        ndarray<const double,2> in (en.content(),shape);
        if (_i<_n)
          pdsalg::variance_accumulate(1./vn,in,_m1[0],_m2[0]);
        else {
          EntryTH1F& ca = static_cast<EntryTH1F&>(*_cache);
          ndarray<double,2> out(ca.content(),shape);
          pdsalg::variance_calculate (1./vn,in,_m1[0],_m2[0],_i,out);
          _cache->valid(e.time());
          if (_n) {
            _i = 0;
            _zero(_m1[0],_m2[0]);
          }
        }
      } break;
    case DescEntry::Waveform: 
      { const EntryWaveform& en = static_cast<const EntryWaveform&>(e);
        unsigned shape[] = {1,en.desc().nbins()};
        ndarray<const double,2> in (en.content(),shape);
        if (_i<_n)
          pdsalg::variance_accumulate(1./vn,in,_m1[0],_m2[0]);
        else {
          EntryWaveform& ca = static_cast<EntryWaveform&>(*_cache);
          ndarray<double,2> out(ca.content(),shape);
          pdsalg::variance_calculate (1./vn,in,_m1[0],_m2[0],_i,out);
          _cache->valid(e.time());
          if (_n) {
            _i = 0;
            _zero(_m1[0],_m2[0]);
          }
        }
      } break;
    case DescEntry::Image:
      { const EntryImage& en = static_cast<const EntryImage&>(e);
        EntryImage& ca = static_cast<EntryImage&>(*_cache);
        const DescImage& d = en.desc();
        double ped = en.info(EntryImage::Pedestal);
        if (d.nframes()) {
          int k;
#ifdef _OPENMP
#pragma omp parallel private(k) num_threads(4)
          {
#pragma omp for schedule(dynamic,1) nowait
#else
          {
#endif
            for(k=0; k<int(d.nframes()); k++) {
              ndarray<const unsigned,2> in (en.contents(k));
              if (_i<_n)
                pdsalg::variance_accumulate(1./vn,ped,in,_m1[k],_m2[k]);
              else {
                ndarray<unsigned,2>       out(ca.contents(k));
                pdsalg::variance_calculate (1./vn,ped,in,_m1[k],_m2[k],_i,out);
                if (_n)
                  _zero(_m1[k],_m2[k]);
              }
            }
          } // for schedule
        }
        else {
          ndarray<const unsigned,2> in (en.content());
          if (_i<_n)
            pdsalg::variance_accumulate(1./vn,ped,in,_m1[0],_m2[0]);
          else {
            ndarray<unsigned,2>       out(ca.content());
            pdsalg::variance_calculate (1./vn,ped,in,_m1[0],_m2[0],_i,out);
            if (_n)
              _zero(_m1[0],_m2[0]);
          }
        }
        if (_i>=_n) {
          if (_n) _i=0;
          _cache->valid(e.time());
        }
      } break;
    default:
      break;
    }
  }

  return *_cache;
}

