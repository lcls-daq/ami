#include "TimeToolM.hh"

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

using namespace Ami;

#include "pdsdata/psddl/encoder.ddl.h"

#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <string>
#include <fstream>

using std::string;

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
    FexM(const char* fname) :
      Fex(fname),
      _frame(0),
      _evrdata(0),
      _ipmdata(0),
      _cds(0) {}
    ~FexM() {}
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
      cache.add(base_name()+":AMI:SIGROISUM");
      return _cache_index;
    }
    void configure() {
      TimeTool::Fex::configure();
      if (_frame)
        _frame->clear_frame();
      _evrdata = 0;
      _ipmdata = 0;
    }
    void configure(const Pds::Epics::EpicsPvCtrlHeader& pv) {
      /*
      const int slen = base_name().length();
      if (strncmp(pv.pvName(),base_name().c_str(),slen)==0) {
        if      (strcmp(&pv.pvName()[slen],":SIGNAL_WF")==0)
          _id_sig = pv.pvId();
        else if (strcmp(&pv.pvName()[slen],":SIDEBAND_WF")==0)
          _id_sb  = pv.pvId();
        else if (strcmp(&pv.pvName()[slen],":REFERENCE_WF")==0)
          _id_ref = pv.pvId();
      }
      */
    }
    void event(const Pds::DetInfo&   src,
               const Pds::TypeId&    type,
               void*                 payload) {
      if (src.phy()==_src.phy()) {
        if (type.id()==Pds::TypeId::Id_Frame) {
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
        }
        else if (type.id()==Pds::TypeId::Id_VimbaFrame) {
          if (_frame)
            _frame->set_frame(type, payload);
        }
      }
      else if (type.id()==Pds::TypeId::Id_EvrData) {
        // Pds::EvrData::DataV3 can be used for both V3 and V4
        _evrdata = reinterpret_cast<Pds::EvrData::DataV3*>(payload);
      }
      else if (src.level()==m_ipm_get_key.level() && 
               src.phy  ()==m_ipm_get_key.phy  () &&
               type.id  ()==Pds::TypeId::Id_IpmFex)
        _ipmdata = reinterpret_cast<Pds::Lusi::IpmFexV1*>(payload);
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
      _cds=0;
      TimeTool::Fex::configure();
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
      _cols=cols;
      _cds=&cds;
    }
    void analyze(const Pds::ClockTime& _clk) {
      //printf("analyze[%08x] evrdata %p  frame %p\n",
      //       _src.phy(), _evrdata, _frame);
      if (_evrdata && _frame && !_frame->empty()) {
        m_pedestal = _frame->offset();
        TimeTool::Fex::analyze(_frame->data(),
                               _evrdata->fifoEvents(),
                               _ipmdata);

        if (_cache) {
          _cache->cache(_cache_index+0, amplitude());
          _cache->cache(_cache_index+1, filtered_position());
          _cache->cache(_cache_index+2, filtered_pos_ps());
          _cache->cache(_cache_index+3, filtered_fwhm());
          _cache->cache(_cache_index+4, next_amplitude());
          _cache->cache(_cache_index+5, ref_amplitude());
          _cache->cache(_cache_index+6, sig_roi_sum());
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

      //  Reset pointer references
      if (_frame)
        _frame->clear_frame();
      _evrdata = 0;
      _ipmdata = 0;
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

    TimeTool::FrameCache* _frame;
    Pds::EvrData::DataV3* _evrdata;
    Pds::Lusi::IpmFexV1*  _ipmdata;

    int         _cols;
    int         _cols_offset;
    Cds*        _cds;

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
  char buff[PATH_MAX];
  const char* dir = TimeTool::default_file_path();
  sprintf(buff,"%s/%s", dir, fname);
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
  if (type.id()==Pds::TypeId::Id_Opal1kConfig || type.id()==Pds::TypeId::Id_AlviumConfig) {
    for(unsigned i=0; i<_fex.size(); i++)
      if (_fex[i]->src().phy() == src.phy()) {
        _fex[i]->set_frame(TimeTool::FrameCache::instance(src, type, payload));
        _fex[i]->configure();
      }
  }
  else if (type.id()==Pds::TypeId::Id_Epics) {
    for(unsigned i=0; i<_fex.size(); i++) {
      FexM& fex = *_fex[i];
      if (src.phy() == fex.src().phy())
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
