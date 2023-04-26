#include "TimeToolE.hh"

#include "ami/data/Cds.hh"

#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryTH1F.hh"

#include "timetool/service/Fex.hh"
#include "timetool/service/FrameCache.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/compress/CompressedXtc.hh"
#include "pdsdata/psddl/epics.ddl.h"

#include "pdsdata/psddl/camera.ddl.h"
#include "pdsdata/psddl/opal1k.ddl.h"
#include "pdsdata/psddl/evr.ddl.h"
#include "pdsdata/psddl/lusi.ddl.h"
#include "pdsdata/psddl/timetool.ddl.h"

using namespace Ami;

#include "pdsdata/psddl/encoder.ddl.h"

#include <math.h>
#include <stdlib.h>
#include <string>
#include <fstream>

using std::string;

typedef Pds::TimeTool::ConfigV3 TimeToolConfigType;
typedef std::map<Pds::Src, TimeTool::FrameCache*> FrameCacheMap;
typedef FrameCacheMap::iterator FrameCacheIter;

static const char* op_name[] = { "OR", "AND", "OR_NOT", "AND_NOT", NULL };

static void dump_logic(const ndarray<const Pds::TimeTool::EventLogic,1>& logic)
{
  for(unsigned i=0; i<logic.size(); i++)
    printf("%s %u", op_name[logic[i].logic_op()], logic[i].event_code());
  printf("\n");
}

static void dump(const TimeToolConfigType& c)
{
  printf("--beam logic--\n");
  dump_logic(c.beam_logic());
  printf("--laser logic--\n");
  dump_logic(c.laser_logic());
}

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
    FexM(const Pds::Src& src,
         const TimeToolConfigType& cfg) : 
      Fex(src,cfg,false), _cols(0), _cds(0), _cache(0),
      _config_buffer(new char[cfg._sizeof()]),
      _data(0),
      _frame(0),
      _evrdata(0),
      _ipmdata(0)
    {
      dump(cfg); 
      memcpy(_config_buffer, &cfg, cfg._sizeof());
      reset_data();
    }
    ~FexM() { clear(_cds); delete[] _config_buffer; }
  public:
    void _monitor_raw_sig(const ndarray<const double,1>& wf)
    {
      for(unsigned k=0; k<wf.size(); k++)
        _raw_signal->content(wf[k],k);
    }
    void _monitor_ref_sig(const ndarray<const double,1>& wf)
    {
      for(unsigned k=0; k<wf.size(); k++)
        _ref_signal->content(wf[k],k);
      printf("\n");
    }
    void _monitor_sub_sig (const ndarray<const double,1>& wf)
    {
      for(unsigned k=0; k<wf.size(); k++)
        _sub_signal->content(wf[k],k);
    }
    void _monitor_flt_sig (const ndarray<const double,1>& wf)
    {
      unsigned mcols = unsigned(_cols - m_flt_offset);
      for(unsigned k=0; k<wf.size() && k<mcols; k++)
        _flt_signal->content(wf[k],k+m_flt_offset);
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
    void reset_data() {
      //  Reset pointer references
      _data = 0;
      if (_frame)
        _frame->clear_frame();
      _evrdata = 0;
      _ipmdata = 0;
    }
    void event(const Pds::DetInfo&   src,
               const Pds::TypeId&    type,
               void*                 payload) {
      switch(type.id()) {
      case Pds::TypeId::Id_TimeToolData:
        if (src.phy()==_src.phy())
          _data = reinterpret_cast<Pds::TimeTool::DataV3*>(payload);
        break;
      case Pds::TypeId::Id_Frame:
        if (src.phy()==_src.phy()) {
          if (type.compressed()) {
            const Pds::Xtc* xtc = reinterpret_cast<const Pds::Xtc*>(payload)-1;
            _pXtc = Pds::CompressedXtc::uncompress(*xtc);
            if (_frame)
              _frame->set_frame(type, _pXtc->payload());
          }
          else {
            if (_frame)
              _frame->set_frame(type, payload);
          }
        } break;
      case Pds::TypeId::Id_VimbaFrame:
        if (src.phy()==_src.phy()) {
          if (_frame)
            _frame->set_frame(type, payload);
        } break;
      case Pds::TypeId::Id_EvrData:
        // Pds::EvrData::DataV3 can be used for both V3 and V4
        _evrdata = reinterpret_cast<Pds::EvrData::DataV3*>(payload);
        break;
      case Pds::TypeId::Id_IpmFex:
        if (src.level()==m_ipm_get_key.level() && 
            src.phy  ()==m_ipm_get_key.phy  ())
          _ipmdata = reinterpret_cast<Pds::Lusi::IpmFexV1*>(payload);
        break;
      default:
        break;
      }
    }
    void clear(Cds* cds) {
      if (_cds) {
        _cds->remove(_ref_signal);
        _cds->remove(_raw_signal);
        _cds->remove(_sub_signal);
        _cds->remove(_flt_signal);
        _cds->remove(_indicator);
        delete _ref_signal;
        delete _raw_signal;
        delete _sub_signal;
        delete _flt_signal;
        delete _indicator;
      }
      _cds = 0;
    }
    void create(Cds& cds, unsigned column) {
      char buff[128];
      const int   pdim  = m_projectX ? 1:0;
      const int   ilo   = m_sig_roi_lo[pdim];
      const int   ihi   = m_sig_roi_hi[pdim];
      const int   cols  = ihi-ilo+1;
      const float lo(ilo);
      const float hi(ihi);
      sprintf(buff,"Ref Signal#Signal#0#%d#0",column);
      _ref_signal  = one_evt(DescTH1F(buff,"ADU","Bin",cols,lo,hi));
      sprintf(buff,"Raw Signal#Signal#0#%d#c0",column);
      _raw_signal  = one_evt(DescTH1F(buff,"ADU","Bin",cols,lo,hi));
      sprintf(buff,"Sub Signal#Signal#1#%d#c0",column);
      _sub_signal  = one_evt(DescTH1F(buff,"ADU","Bin",cols,lo,hi));
      sprintf(buff,"Flt Signal#Signal#1#%d#c00000",column);
      _flt_signal  = one_evt(DescTH1F(buff,"ADU","Bin",cols,lo,hi));
      sprintf(buff,"Flt Pos#Signal#1#%d#00c000",column);
      _indicator   = one_evt(DescTH1F(buff,"ADU","Bin",cols,lo,hi));
      cds.add(_ref_signal);
      cds.add(_raw_signal);
      cds.add(_sub_signal);
      cds.add(_flt_signal);
      cds.add(_indicator);
      _cols_offset=ilo;
      _cols= cols;
      _cds = &cds;
      init_plots();
    }
    void analyze(const Pds::ClockTime& _clk) {
      const TimeToolConfigType& c = 
        *reinterpret_cast<const TimeToolConfigType*>(_config_buffer);
      if ((_data && (_data->projected_signal(c).size() || _data->full_signal(c).size())) ||
          (_evrdata && _frame && !_frame->empty())) {

        reset();

        if (_data && _data->projected_signal(c).size()) {
          if (c.use_reference_roi())
            TimeTool::Fex::analyze(TimeTool::Fex::EventType(_data->event_type()),
                                   _data->projected_signal(c),
                                   _data->projected_sideband(c),
                                   _data->projected_reference(c));
          else
            TimeTool::Fex::analyze(TimeTool::Fex::EventType(_data->event_type()),
                                   _data->projected_signal(c),
                                   _data->projected_sideband(c));
        }
        else if (_data && _data->full_signal(c).size()) {
          if (c.use_reference_roi())
            TimeTool::Fex::analyze(TimeTool::Fex::EventType(_data->event_type()),
                                   _data->full_signal(c),
                                   _data->full_sideband(c),
                                   _data->full_reference(c));
          else
            TimeTool::Fex::analyze(TimeTool::Fex::EventType(_data->event_type()),
                                   _data->full_signal(c),
                                   _data->full_sideband(c));
        }
        else {
          m_pedestal = _frame->offset();
          TimeTool::Fex::analyze(_frame->data(),
                                 _evrdata->fifoEvents(),
                                 _ipmdata);
        }

        if (_cache) {
          _cache->cache(_cache_index+0, amplitude());
          _cache->cache(_cache_index+1, filtered_position());
          _cache->cache(_cache_index+2, filtered_pos_ps());
          _cache->cache(_cache_index+3, filtered_fwhm());
          _cache->cache(_cache_index+4, next_amplitude());
          _cache->cache(_cache_index+5, ref_amplitude());
        }

        if (status()) {
          double min = 0, max = 0;
          for (int n=0; n<_cols; n++) {
            double cur_sig = _sub_signal->content(n);
            double cur_flt = _flt_signal->content(n);
            if (n==0) {
              min = cur_sig;
              max = cur_sig;
            } else {
              if (cur_sig > max) max = cur_sig;
              if (cur_flt > max) max = cur_flt;
              if (cur_sig < min) min = cur_sig;
              if (cur_flt < min) min = cur_flt;
            }
          }
          for (int i=0; i<_cols; i++) {
            _indicator->content(min, i);
          }

          int ix = int(filtered_position()-_cols_offset);
          if (ix>=0 && ix<_cols)
            _indicator->content(max,ix);
        }

        _ref_signal->valid(_clk);
        _raw_signal->valid(_clk);
        _sub_signal->valid(_clk);
        _flt_signal->valid(_clk);
        _indicator->valid(_clk);
      }

      reset_data();
    }
    void set_frame(TimeTool::FrameCache* frame) {
      // replace old frame cache with new one
      if (_frame)
        delete _frame;
      _frame = frame;
    }
  private:
    EntryTH1F* _ref_signal;
    EntryTH1F* _raw_signal;
    EntryTH1F* _sub_signal;
    EntryTH1F* _flt_signal;
    EntryTH1F* _indicator;

    int                  _cols;
    int                  _cols_offset;
    Cds*                 _cds;
    Ami::FeatureCache*   _cache;
    int                  _cache_index;
    boost::shared_ptr<Pds::Xtc> _pXtc;

    char* _config_buffer;
    Pds::TimeTool::DataV3* _data;
    TimeTool::FrameCache*  _frame;
    Pds::EvrData::DataV3*  _evrdata;
    Pds::Lusi::IpmFexV1*   _ipmdata;
  };
};


TimeToolE::TimeToolE() : 
  _cds      (0),
  _cache    (0)
{
}

TimeToolE::~TimeToolE() 
{
  for(unsigned i=0; i<_fex.size(); i++)
    delete _fex[i];
  _fex.clear();
  for(FrameCacheIter it=_tmp.begin(); it!=_tmp.end(); ++it) {
    if (it->second)
      delete it->second;
  }
  _tmp.clear();
}

void TimeToolE::reset(Ami::FeatureCache& cache)
{
  for(unsigned i=0; i<_fex.size(); i++)
    delete _fex[i];
  _fex.clear();
  for(FrameCacheIter it=_tmp.begin(); it!=_tmp.end(); ++it) {
    if (it->second)
      delete it->second;
  }
  _tmp.clear();

  _cache = &cache;
}

void TimeToolE::clock    (const Pds::ClockTime& clk) 
{
//   if (clk.seconds() > _clk.seconds() &&
//       (clk.seconds()&0xf)==0)
//     recreate();

  _clk=clk; 
}

//
//  Cache the configuration for camera
//
void TimeToolE::configure(const Pds::DetInfo&   src,
                          const Pds::TypeId&    type,
                          void*                 payload)
{
  if (type.id()==Pds::TypeId::Id_TimeToolConfig) {
    FexM* f = new FexM(src,*reinterpret_cast<const TimeToolConfigType*>(payload));
    if (_cds)
      f->create(*_cds,_fex.size());
    if (_cache)
      f->cache(*_cache);
    _fex.push_back(f);

    FrameCacheIter it = _tmp.find(src);
    if (it != _tmp.end()) {
      // add framecache to fex
      f->set_frame(it->second);
      // remove framecache from tmp map
      _tmp.erase(it);
    }
  }
  else if (type.id()==Pds::TypeId::Id_Opal1kConfig || type.id()==Pds::TypeId::Id_AlviumConfig) {
    bool found = false;
    for(unsigned i=0; i<_fex.size(); i++)
      if (_fex[i]->src().phy() == src.phy()) {
        _fex[i]->set_frame(TimeTool::FrameCache::instance(src, type, payload));
        found = true;
      }
    if (!found) {
      _tmp[src] = TimeTool::FrameCache::instance(src, type, payload);
    }
  }
}

//
//  Capture pointer to detector data we want
//
void TimeToolE::event    (const Pds::DetInfo&   src,
                          const Pds::TypeId&    type,
                          const Pds::Damage&    damage,
                          void*                 payload)
{
  if (damage.value()) return;

  if (_cds)
    for(unsigned i=0; i<_fex.size(); i++)
      _fex[i]->event(src,type,payload);
}

//
//  Remove all plot entries
//
void TimeToolE::clear    () 
{
  for(unsigned i=0; i<_fex.size(); i++)
    _fex[i]->clear(_cds);
  _cds = 0;
}

void TimeToolE::create   (Cds& cds)
{
  if (!_cds)
    for(unsigned i=0; i<_fex.size(); i++)
      _fex[i]->create(cds,i);
  _cds = &cds; 
}

//
//  Analyze data for current event from the pointers we captured
//
void TimeToolE::analyze  ()
{
  if (_cds)
    for(unsigned i=0; i<_fex.size(); i++)
      _fex[i]->analyze(_clk);
}


// FROM FILTER
const char* TimeToolE::name() const { return "TimeTool"; }

//
//  Analyze data for current event from the pointers we captured
//
bool TimeToolE::accept  () 
{
  analyze();
  return true;
}


//
//  Plug-in module creator
//

extern "C" UserModule* create() { return new TimeToolE; }

extern "C" void destroy(UserModule* p) { delete p; }
