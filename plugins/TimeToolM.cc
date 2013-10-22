#include "TimeToolM.hh"

#include "ami/data/Cds.hh"

#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryTH1F.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
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

using std::string;

typedef Pds::Opal1k::ConfigV1 Opal1kConfig;
typedef Pds::EvrData::DataV3 EvrDataType;

#include "timetool/service/Fex.hh"
#include "timetool/service/RatioFit.hh"

//#define DBUG

static const int   cols  = Pds::Opal1k::ConfigV1::Column_Pixels;

namespace Ami {

  class FexM : public TimeTool::Fex {
  public:
    FexM() {}
    ~FexM() {}
  public:
    void _monitor_raw_sig(const uint32_t* wf, double* sb) 
    {
      for(int k=_spec_begin; k<_spec_end; k++)
        _raw_hist->content(double(wf[k])-sb[k],k);
    }
    void _monitor_ref_sig(const double* wf) 
    {
      for(int k=_spec_begin; k<_spec_end; k++)
        _ref_hist->content(wf[k],k);
    }
    void _monitor_sub_sig (const double* wf)
    {
      for(int k=_spec_begin; k<_spec_end; k++)
        _sub_hist->content(wf[k],k);
    }
    void _monitor_sub_sig_u (const double* wf)
    {
      for(int k=_spec_begin; k<_spec_end; k++)
        _sub_hist_u->content(wf[k],k);
    }
    void _monitor_flt_sig (const double* wf)
    {
      for(int k=_spec_begin+_nwts; k<_spec_end; k++)
        _flt_hist->content(wf[k],k);
    }
    void _monitor_corr    (const TimeTool::RatioFit& f0,
                           const TimeTool::RatioFit& f1)
    {
      _p0_corr->addy(f1.p0(),f0.p0());
      _p1_corr->addy(f1.p1(),f0.p1());
    }
  public:
    void ref_sig (EntryTH1F* h) { _ref_hist=h; }
    void raw_sig (EntryTH1F* h) { _raw_hist=h; }
    void sub_sig (EntryTH1F* h) { _sub_hist=h; }
    void sub_sig_u (EntryTH1F* h) { _sub_hist_u=h; }
    void flt_sig (EntryTH1F* h) { _flt_hist=h; }
    void p0_corr (EntryScan* h) { _p0_corr=h; }
    void p1_corr (EntryScan* h) { _p1_corr=h; }
  private:
    EntryTH1F* _ref_hist;
    EntryTH1F* _raw_hist;
    EntryTH1F* _sub_hist;
    EntryTH1F* _sub_hist_u;
    EntryTH1F* _flt_hist;
    EntryScan* _p0_corr;
    EntryScan* _p1_corr;
  };
};


TimeToolM::TimeToolM() : 
  _cds(0),
  _fex(new FexM),
  _frame    (0),
  _evrdata  (0),
  _ipmdata  (0),
  _sig_wf   (0),
  _sb_wf    (0),
  _ref_wf   (0),
  _cache    (0)
 {}
TimeToolM::~TimeToolM() 
{
  delete _fex;
}

void TimeToolM::reset(Ami::FeatureCache& cache)
{
  _cache = &cache;
  _fex->configure();

  if (_cds) {
    int index = _cache->add(_fex->base_name()+":AMI:AMPL"); 
    _cache->add(_fex->base_name()+":AMI:FLTPOS"); 
    _cache->add(_fex->base_name()+":AMI:FLTPOS_PS"); 
    _cache->add(_fex->base_name()+":AMI:FLTPOSFWHM"); 
    _cache->add(_fex->base_name()+":AMI:AMPLNXT"); 
    _cache->add(_fex->base_name()+":AMI:REFAMPL"); 
    _cache_index = index;
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
    _fex->configure();
  }
  else if (src.phy() == _fex->_phy) {
    if (type.id()==Pds::TypeId::Id_Epics) {
      const int slen = _fex->base_name().length();
      const Pds::Epics::EpicsPvCtrlHeader& pv = 
        *reinterpret_cast<Pds::Epics::EpicsPvCtrlHeader*>(payload);
      if (strncmp(pv.pvName(),_fex->base_name().c_str(),slen)==0) {
	if      (strcmp(&pv.pvName()[slen],":SIGNAL_WF")==0)
	  _id_sig = pv.pvId();
	else if (strcmp(&pv.pvName()[slen],":SIDEBAND_WF")==0)
	  _id_sb  = pv.pvId();
	else if (strcmp(&pv.pvName()[slen],":REFERENCE_WF")==0)
	  _id_ref = pv.pvId();
      }
    }
  }
}

//
//  Capture pointer to detector data we want
//
void TimeToolM::event    (const Pds::DetInfo&   src,
			  const Pds::TypeId&    type,
			  void*                 payload) 
{
  if (src.phy()==_fex->_phy) {
    if (type.id()==Pds::TypeId::Id_Frame)
      _frame = reinterpret_cast<Pds::Camera::FrameV1*>(payload);
    else if (type.id()==Pds::TypeId::Id_Epics) {
      const Pds::Epics::EpicsPvTimeLong& pv = *reinterpret_cast<const Pds::Epics::EpicsPvTimeLong*>(payload);
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
  else if (src.level()==_fex->_ipm_no_beam_src.level() && 
	   src.phy  ()==_fex->_ipm_no_beam_src.phy  () &&
	   type.id  ()==Pds::TypeId::Id_IpmFex)
    _ipmdata = reinterpret_cast<Pds::Lusi::IpmFexV1*>(payload);
}

//
//  Remove all plot entries
//
void TimeToolM::clear    () 
{
  if (_cds) {
    _cds->remove(_ref_signal);
    _cds->remove(_raw_signal);
    _cds->remove(_sub_signal);
    _cds->remove(_sub_signal_u);
    _cds->remove(_flt_signal);
    _cds->remove(_p0corr);
    _cds->remove(_p1corr);
    delete _ref_signal;
    delete _raw_signal;
    delete _sub_signal;
    delete _sub_signal_u;
    delete _flt_signal;
    delete _p0corr;
    delete _p1corr;
  }
  _cds = 0;

  _fex->configure();
}

#if 0
static DescTH1F no_agg(DescTH1F d) { d.aggregate(false); return d; }
#endif

//
//  Create all plot entries
//
static EntryTH1F* one_evt(DescTH1F d) {
  d.aggregate(false);
  return new EntryTH1F(d);
}

void TimeToolM::create   (Cds& cds)
{
  _cds = &cds; 

  _ref_signal  = one_evt(DescTH1F("Ref Signal#Signal#0#0#0"     ,"ADU","Bin",cols,0.,double(cols)));
  _raw_signal  = one_evt(DescTH1F("Raw Signal#Signal#0#0#c0"    ,"ADU","Bin",cols,0.,double(cols)));
  _sub_signal  = one_evt(DescTH1F("Sub Signal#Signal#1#0#c0"    ,"ADU","Bin",cols,0.,double(cols)));
  _sub_signal_u  = one_evt(DescTH1F("Sub Signal U#Signal#1#0#c000"    ,"ADU","Bin",cols,0.,double(cols)));
  _flt_signal  = one_evt(DescTH1F("Flt Signal#Signal#1#0#c00000","ADU","Bin",cols,0.,double(cols)));

  _p0corr      = new EntryScan(DescScan("P0#Reference","Reference","Signal",100));
  _p1corr      = new EntryScan(DescScan("P1#Reference","Reference","Signal",100));

  cds.add(_ref_signal);
  cds.add(_raw_signal);
  cds.add(_sub_signal);
  cds.add(_sub_signal_u);
  cds.add(_flt_signal);
  cds.add(_p0corr);
  cds.add(_p1corr);

  if (_cache) {
    int index = _cache->add(_fex->base_name()+":AMI:AMPL"); 
    _cache->add(_fex->base_name()+":AMI:FLTPOS"); 
    _cache->add(_fex->base_name()+":AMI:FLTPOS_PS"); 
    _cache->add(_fex->base_name()+":AMI:FLTPOSFWHM"); 
    _cache->add(_fex->base_name()+":AMI:AMPLNXT"); 
    _cache->add(_fex->base_name()+":AMI:REFAMPL"); 
    _cache_index = index;
    printf("TimeToolM::create _cache_index=%d\n",index);
  }
  else
    printf("TimeToolM::create _cache==0\n");

  _fex->raw_sig(_raw_signal);
  _fex->ref_sig(_ref_signal);
  _fex->sub_sig(_sub_signal);
  _fex->sub_sig_u(_sub_signal_u);
  _fex->flt_sig(_flt_signal);
  _fex->p0_corr(_p0corr);
  _fex->p1_corr(_p1corr);

  _fex->init_plots();
}

//
//  Analyze data for current event from the pointers we captured
//
void TimeToolM::analyze  ()
{
  if (_cds) {

    if (_evrdata) {

      bool bykik = false;
      bool no_laser = false;
      unsigned laser_code = abs(_fex->_event_code_no_laser);
      for(unsigned i=0; i<_evrdata->numFifoEvents(); i++) {
        const Pds::EvrData::FIFOEvent& fe = _evrdata->fifoEvents()[i];
        if (fe.eventCode() == _fex->_event_code_bykik ||
            fe.eventCode() == _fex->_event_code_alkik)
          bykik = true;
        if (fe.eventCode() == laser_code)
          no_laser = true;
      }

      if (int(_fex->_event_code_no_laser) < 0)
        no_laser = !no_laser;

      if (_ipmdata) {
	if (_ipmdata->sum() < _fex->_ipm_no_beam_threshold)
	  bykik = true;
      }

      bool lpass = _fex->write_image() && _frame;
      if (lpass)
        _fex->analyze(*_frame, bykik, no_laser);
      else if ((lpass = _fex->write_projections() && _sig_wf))
        _fex->analyze(_sig_wf, _sb_wf, _ref_wf, bykik, no_laser);
      else if ((lpass = _frame))
        _fex->analyze(*_frame, bykik, no_laser);

      if (lpass) {

#ifdef DBUG
        printf("fex status %c  ampl %f  pos %f  ref_a %f  next_a %f\n",
               _fex->status() ? 't':'f',
               _fex->amplitude(),
               _fex->filtered_position(),
               _fex->ref_amplitude(),
               _fex->next_amplitude());
#endif

        if (_cache) {
          _cache->cache(_cache_index+0, _fex->amplitude());
          _cache->cache(_cache_index+1, _fex->filtered_position());
          _cache->cache(_cache_index+2, _fex->filtered_pos_ps());
          _cache->cache(_cache_index+3, _fex->filtered_fwhm());
          _cache->cache(_cache_index+4, _fex->next_amplitude());
          _cache->cache(_cache_index+5, _fex->ref_amplitude());
        }

        if (_fex->status()) {
          int ix = int(_fex->filtered_position()+_fex->_indicator_offset);
          if (ix>=0 && ix<cols)
            _sub_signal->content(1.,ix);
        }

        _ref_signal->valid(_clk);
        _raw_signal->valid(_clk);
        _sub_signal->valid(_clk);
        _sub_signal_u->valid(_clk);
        _flt_signal->valid(_clk);
        _p0corr    ->valid(_clk);
        _p1corr    ->valid(_clk);

      }
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
