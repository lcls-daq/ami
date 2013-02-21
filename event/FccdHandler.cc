#include "FccdHandler.hh"

#include "ami/event/FccdCalib.hh"
#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/fccd/FccdConfigV1.hh"
#include "pdsdata/fccd/FccdConfigV2.hh"
#include "pdsdata/camera/FrameV1.hh"

#include <string.h>
#include <stdio.h>

using namespace Ami;

static const unsigned Rows   = Pds::FCCD::FccdConfigV2::Trimmed_Row_Pixels;
static const unsigned Cols   = Pds::FCCD::FccdConfigV2::Trimmed_Column_Pixels;
static const unsigned Offset = 0x10000;
static unsigned _zero_pedestals[Rows*Cols];


FccdHandler::FccdHandler(const Pds::DetInfo& info) : 
  EventHandler(info, Pds::TypeId::Id_Frame, Pds::TypeId::Id_FccdConfig),
  _entry(0),
  _pedestals(new unsigned[Rows*Cols])
{
}

FccdHandler::FccdHandler(const Pds::DetInfo& info, const EntryImage* entry) : 
  EventHandler(info, Pds::TypeId::Id_Frame, Pds::TypeId::Id_FccdConfig),
  _entry(entry ? new EntryImage(entry->desc()) : 0),
  _pedestals(new unsigned[Rows*Cols])
{
}

FccdHandler::~FccdHandler()
{
  delete[] _pedestals;
}

unsigned FccdHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* FccdHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void FccdHandler::reset() { _entry = 0; }

void FccdHandler::_configure(Pds::TypeId type, const void* payload, const Pds::ClockTime& t)
{
  _load_pedestals();

  switch(type.version()) {
  case 1: {
    const Pds::FCCD::FccdConfigV1& c = *reinterpret_cast<const Pds::FCCD::FccdConfigV1*>(payload);
    const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
    DescImage desc(det, (unsigned)0, ChannelID::name(det),
		   c.trimmedWidth(),
		   c.trimmedHeight()); // FCCD image is 480 x 480 after removing dark pixels
    _entry = new EntryImage(desc);
  } break;
  case 2: {
    const Pds::FCCD::FccdConfigV2& c = *reinterpret_cast<const Pds::FCCD::FccdConfigV2*>(payload);
    const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
    DescImage desc(det, (unsigned)0, ChannelID::name(det),
		   c.trimmedWidth(),
		   c.trimmedHeight()); // FCCD image is 480 x 480 after removing dark pixels
    _entry = new EntryImage(desc);
  } break;
  default:
    break;
  }

}

void FccdHandler::_calibrate(Pds::TypeId::Type, const void* payload, const Pds::ClockTime& t) {}

void FccdHandler::_event    (const void* payload, const Pds::ClockTime& t)
{
  const Pds::Camera::FrameV1& f = *reinterpret_cast<const Pds::Camera::FrameV1*>(payload);
  if (!_entry) return;

  //  memset(_entry->contents(),0,_entry->desc().nbinsx()*_entry->desc().nbinsy()*sizeof(unsigned));
  const uint16_t* d  = reinterpret_cast<const uint16_t*>(f.data());

  static unsigned _options = -1;
  if (_entry->desc().options() != _options) {
    printf("FccdHandler options %x -> %x\n",_options,_entry->desc().options());
    _options = _entry->desc().options();
  }
	   
  if (_entry->desc().options() & FccdCalib::option_reload_pedestal()) {
    _load_pedestals();
    _entry->desc().options( _entry->desc().options()&~FccdCalib::option_reload_pedestal() );
  }

  bool lPeds = (_entry->desc().options()&FccdCalib::option_no_pedestal()) == 0;
  const unsigned* ped = lPeds ? _pedestals : _zero_pedestals;

  //  Height:
  //    500 rows   = 6 + 240 * 7 + 240 + 7
  //         Dark A: 6   Rows 0-5
  //       Data Top: 240 Rows 6-245
  //         Dark B: 7   Rows 246-252
  //    Data Bottom: 240 Rows 253-492
  //         Dark C: 7   Rows 493-249
  // 
  //  Width (in 16-bit pixels):
  //    576 pixels = 12 * 48 outputs
  //            Top: (10 image pixels followed by 2 info pixels) * 48 outputs
  //         Bottom: (2 info pixels followed by 10 image pixels) * 48 outputs

  unsigned i, j, k;

  // 6 top info rows skipped (Dark A)
  d += (6 * f.width() / 2);    // f.width() is in 8 bit pixels

  // top half of image -- 240 rows per half
  for (i = 6; i < 246; i++) {
    // 48 outputs
    for (j = 0; j < 48; j++) {
      // 10 image pixels per output
      for (k = 0; k < 10; k++) {
        _entry->content(*ped+*d, (j*10)+k, i-6);  // 6 rows skipped above
        ped++;
        d++;
      }
      // 2 info pixels per output (skipped)
      d += 2;
    }
  }
  // 7 middle info rows skipped (Dark B)
  d += (7 * f.width() / 2);    // f.width() is in 8 bit pixels

  // bottom half of image -- 240 rows per half
  for (i = 253; i < 493; i++) {
    // 48 outputs
    for (j = 0; j < 48; j++) {
      // 2 info pixels per output (skipped)
      d += 2;
      // 10 image pixels per output
      for (k = 0; k < 10; k++) {
        _entry->content(*ped+*d, (j*10)+k, i-13); // 13 rows skipped above
        ped++;
        d++;
      }
    }
  }

  _entry->info(lPeds ? Offset: 0,EntryImage::Pedestal);
  _entry->info(1,EntryImage::Normalization);
  _entry->valid(t);
}

void FccdHandler::_damaged() { _entry->invalid(); }

void FccdHandler::_load_pedestals() 
{
  //
  //  Load pedestals
  //
  const int NameSize=128;
  char oname1[NameSize];
  char oname2[NameSize];

  sprintf(oname1,"ped.%08x.dat",info().phy());
  sprintf(oname2,"/reg/g/pcds/pds/fccdcalib/ped.%08x.dat",info().phy());
  FILE *f = Calib::fopen_dual(oname1, oname2, "pedestals");

  if (f) {
    size_t sz = 8 * 1024;
    char* linep = (char *)malloc(sz);
    memset(linep, 0, sz);
    char* pEnd = linep;

    unsigned* ped = _pedestals;
    for(unsigned i=0; i<Rows; i++) {
      getline(&linep, &sz, f);
      *ped++ = Offset - int(strtod(linep,&pEnd));
      for(unsigned j=1; j<Cols; j++)
	*ped++ = Offset - int(strtod(pEnd,&pEnd));
    }

    free(linep);
    fclose(f);
  }
  else {
    unsigned* ped = _pedestals;
    for(unsigned i=0; i<Rows; i++)
      for(unsigned j=0; j<Cols; j++)
	*ped++ = Offset;
  }
}
