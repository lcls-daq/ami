#include "FrameHandler.hh"

#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "ami/event/FrameCalib.hh"
#include "ami/event/Calib.hh"
#include "pdsdata/psddl/camera.ddl.h"
#include "pdsdata/psddl/opal1k.ddl.h"

#include <string.h>

using namespace Ami;

FrameHandler::FrameHandler(const Pds::DetInfo& info,
			   unsigned defColumns,
			   unsigned defRows) : 
  EventHandler(info, Pds::TypeId::Id_Frame, Pds::TypeId::Id_FrameFexConfig),
  _entry(0),
  _defColumns(defColumns),
  _defRows   (defRows)
{
}

FrameHandler::FrameHandler(const Pds::DetInfo& info,
			   const std::list<Pds::TypeId::Type>& config_types,
			   unsigned defColumns,
			   unsigned defRows) : 
  EventHandler(info, Pds::TypeId::Id_Frame, config_types),
  _entry(0),
  _defColumns(defColumns),
  _defRows   (defRows)
{
}

FrameHandler::FrameHandler(const Pds::DetInfo& info, const EntryImage* entry) : 
  EventHandler(info, Pds::TypeId::Id_Frame, Pds::TypeId::Id_FrameFexConfig),
  _entry(entry ? new EntryImage(entry->desc()) : 0)
{
}

FrameHandler::~FrameHandler()
{
}

void FrameHandler::rename(const char* name)
{
  if (_entry)
    _entry->desc().name(name);
}

unsigned FrameHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* FrameHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void FrameHandler::reset() { _entry = 0; }

void FrameHandler::_configure(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  Pds::TypeId::Type type = tid.id();
  if (type == Pds::TypeId::Id_FrameFexConfig) {
    const Pds::Camera::FrameFexConfigV1& c = *reinterpret_cast<const Pds::Camera::FrameFexConfigV1*>(payload);
    const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
    unsigned columns,rows;
    switch(c.forwarding()) {
    case Pds::Camera::FrameFexConfigV1::FullFrame:
      columns = _defColumns;
      rows    = _defRows;
      break;
    case Pds::Camera::FrameFexConfigV1::RegionOfInterest:
      columns = c.roiEnd().column()-c.roiBegin().column();
      rows    = c.roiEnd().row   ()-c.roiBegin().row   ();
      break;
    case Pds::Camera::FrameFexConfigV1::NoFrame:
    default:
      return;
    }
    unsigned ppb = image_ppbin(columns,rows,0);
    DescImage desc(det, (unsigned)0, ChannelID::name(det),
		   columns, rows, ppb, ppb);
    _entry = new EntryImage(desc);
    _entry->invalid();
    _load_pedestals();
  }
}

void FrameHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

#include "pdsdata/xtc/ClockTime.hh"

void FrameHandler::_event    (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  if (!_entry || !_entry->desc().used()) return;

  memset(_entry->contents(),0,_entry->desc().nbinsx()*_entry->desc().nbinsy()*sizeof(unsigned));

  const DescImage& desc = _entry->desc();

  unsigned o = desc.options();
  if (_options != o) {
    printf("FrameHandler options %x -> %x\n",_options,o);
    _options = desc.options();
  }
  
  if (desc.options() & FrameCalib::option_reload_pedestal()) {
    _load_pedestals();
    _entry->desc().options( desc.options()&~FrameCalib::option_reload_pedestal() );
  }
  
  const Pds::Camera::FrameV1& f = *reinterpret_cast<const Pds::Camera::FrameV1*>(payload);
  if (f.depth()>8) {
    if (_pedestals.size() && 
        (desc.options()&FrameCalib::option_no_pedestal())==0) {
      ndarray<const uint16_t,2> fr = f.data16();
      ndarray<uint16_t,2> fc = make_ndarray<uint16_t>(fr.shape()[0],
                                                      fr.shape()[1]);
      for(unsigned i=0; i<fr.shape()[0]; i++) {
        uint16_t*       pc = &fc[i][0];
        const uint16_t* pf = &fr[i][0];
        const int*      pp = &_pedestals[i][0];
        for(unsigned j=0; j<fr.shape()[1]; j++)
          pc[j] = pf[j]-pp[j];
      }
      _entry->content(fc);
    }
    else
      _entry->content(f.data16());
  }
  else {
    if (_pedestals.size() &&
        (desc.options()&FrameCalib::option_no_pedestal())==0) {
      ndarray<const uint8_t,2> fr = f.data8();
      ndarray<uint8_t,2> fc = make_ndarray<uint8_t>(fr.shape()[0],
                                                    fr.shape()[1]);
      for(unsigned i=0; i<fr.shape()[0]; i++) {
        uint8_t*      pc = &fc[i][0];
        const uint8_t* pf = &fr[i][0];
        const int*     pp = &_pedestals[i][0];
        for(unsigned j=0; j<fr.shape()[1]; j++)
          pc[j] = pf[j]-pp[j];
      }
      _entry->content(fc);
    }
    else
      _entry->content(f.data8());
  }
  _entry->info(f.offset()*desc.ppxbin()*desc.ppybin(),EntryImage::Pedestal);
  _entry->info(1,EntryImage::Normalization);
  _entry->valid(t);
}

void FrameHandler::_damaged() { if (_entry) _entry->invalid(); }

void FrameHandler::_load_pedestals()
{
  //
  //  Load pedestals.  The dimensions must match the readout configuration.
  //
  _pedestals = make_ndarray<int>(0U,0U);
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  FILE* fp = Calib::fopen(det, "ped", "pedestals");
  if (fp) {
    ndarray<double,2> pedestals = FrameCalib::load_darray(fp);
    const DescImage& desc = _entry->desc();
    unsigned rows    = desc.nbinsy()*desc.ppybin();
    unsigned columns = desc.nbinsx()*desc.ppxbin();
    if (pedestals.size()) {
      if (pedestals.shape()[0]!=rows ||
          pedestals.shape()[1]!=columns) {
        printf("FrameHandler[%s] retrieved pedestals of size [%d,%d] != image size [%d,%d]\n",
               DetInfo::name(static_cast<const DetInfo&>(info())),
               pedestals.shape()[0],pedestals.shape()[1],
               rows,columns);
      }
      else {
        printf("FrameHandler[%s] retrieved pedestals (%d,%d)\n",
               DetInfo::name(static_cast<const DetInfo&>(info())),
               rows,columns);
        _pedestals = make_ndarray<int>(rows,columns);
        const double* pp = pedestals.data();
        for(int* p = _pedestals.begin(); p!=_pedestals.end(); p++, pp++)
          *p = int(*pp+0.5);
      }
    }
    fclose(fp);
  }        
}
