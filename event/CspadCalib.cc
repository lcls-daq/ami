#include "CspadCalib.hh"

#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"

#include "pdsdata/psddl/cspad.ddl.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <libgen.h>

#define DBUG

using namespace Ami;

static const unsigned _option_no_pedestal         = 0x01;
static const unsigned _option_reload_pedestal     = 0x02;
static const unsigned _option_correct_common_mode = 0x04;
static const unsigned _option_suppress_bad_pixels = 0x08;
static const unsigned _option_correct_gain        = 0x10;
static const unsigned _option_post_integral       = 0x20;
static const unsigned _option_correct_unbonded    = 0x40;

unsigned CspadCalib::option_no_pedestal        () { return _option_no_pedestal; }
unsigned CspadCalib::option_reload_pedestal    () { return _option_reload_pedestal; }
unsigned CspadCalib::option_correct_common_mode() { return _option_correct_common_mode; }
unsigned CspadCalib::option_suppress_bad_pixels() { return _option_suppress_bad_pixels; }
unsigned CspadCalib::option_correct_gain       () { return _option_correct_gain; }
unsigned CspadCalib::option_post_integral      () { return _option_post_integral; }
unsigned CspadCalib::option_correct_unbonded   () { return _option_correct_unbonded; }

std::string CspadCalib::save_pedestals(Entry* e,
                                       bool   prod,
                                       bool   reqfull)
{
  std::string msg;

  const Ami::EntryImage& entry = *static_cast<const Ami::EntryImage*>(e);
  const Ami::DescImage& desc = entry.desc();
  const unsigned nframes = entry.desc().nframes();

  char tbuf[32];
  sprintf(tbuf,"%08x.dat",desc.info().phy());
  std::string oname;

  if (prod)
    oname = std::string("/reg/g/pcds/pds/cspadcalib/ped.") + tbuf;
  else
    oname = std::string("ped.") + tbuf;

  //  rename current pedestal file
  time_t t = time(0);
  strftime(tbuf,32,"%Y%m%d_%H%M%S",localtime(&t));
    
  std::string nname = oname + "." + tbuf;
  rename(oname.c_str(),nname.c_str());

  bool fail=false;
  FILE* fn = fopen(oname.c_str(),"w");
  if (!fn) {
    msg = std::string("Unable to write new pedestal file ") + oname;
    fail=true;
  }
  else if ( reqfull &&
            ((entry.desc().info().device()==Pds::DetInfo::Cspad2x2 && nframes != 2) ||
             (entry.desc().info().device()==Pds::DetInfo::Cspad    && nframes != 32)) ) {
    msg = std::string("Failed.  Must readout entire detector.  Check configuration.");
    fail=true;
    fclose(fn);
  }
  else {
    const unsigned ppx = entry.desc().ppxbin();
    const unsigned ppy = entry.desc().ppybin();
    const double dn    = entry.info(EntryImage::Normalization)*double(ppx*ppy);
    const double doff  = entry.info(EntryImage::Pedestal);

#ifdef DBUG
    {  // dump the first pixel's inputs, output
      const SubFrame& frame = entry.desc().frame(0);
      unsigned ix = frame.x;
      unsigned iy = frame.y+frame.ny-1;
      printf("CspadCalib::save_pedestals:  norm %f  doff %f  content[0] %u  ped[0] %f\n",
             dn, doff,
             entry.content(ix,iy),
             (double(entry.content(ix,iy)-doff)/dn));
    }
#endif

    for(unsigned i=0; i<nframes; i++) {
      const SubFrame& frame = entry.desc().frame(i);

      static const Rotation _tr[] = {  D0  , D90 , D180, D90 ,
                                       D90 , D180, D270, D180,
                                       D180, D270, D0  , D270,
                                       D270, D0  , D90 , D0 };

      switch(frame.r) {
      case D0:
        { unsigned x = frame.x;
          unsigned y = frame.y + frame.ny - 1;
          for(unsigned col=0; col<Pds::CsPad::ColumnsPerASIC; col++) {
            for (unsigned row=0; row < 2*Pds::CsPad::MaxRowsPerASIC; row++)
              fprintf(fn, " %.0f", (double(entry.content(x+col/ppx,y-row/ppy))-doff)/dn);
            fprintf(fn, "\n");
          }
          break; }
      case D90:
        { unsigned x = frame.x;
          unsigned y = frame.y;
          for(unsigned col=0; col<Pds::CsPad::ColumnsPerASIC; col++) {
            for (unsigned row=0; row < 2*Pds::CsPad::MaxRowsPerASIC; row++)
              fprintf(fn, " %.0f", (double(entry.content(x+row/ppx,y+col/ppy))-doff)/dn);
            fprintf(fn, "\n");
          }
          break; }
      case D180:
        { unsigned x = frame.x + frame.nx - 1;
          unsigned y = frame.y;
          for(unsigned col=0; col<Pds::CsPad::ColumnsPerASIC; col++) {
            for (unsigned row=0; row < 2*Pds::CsPad::MaxRowsPerASIC; row++)
              fprintf(fn, " %.0f", (double(entry.content(x-col/ppx,y+row/ppy))-doff)/dn);
            fprintf(fn, "\n");
          }
          break; }
      case D270:
        { unsigned x = frame.x + frame.nx - 1;
          unsigned y = frame.y + frame.ny - 1;
          for(unsigned col=0; col<Pds::CsPad::ColumnsPerASIC; col++) {
            for (unsigned row=0; row < 2*Pds::CsPad::MaxRowsPerASIC; row++)
              fprintf(fn, " %.0f", (double(entry.content(x-row/ppx,y-col/ppy))-doff)/dn);
            fprintf(fn, "\n");
          }
          break; }
      default:
        break;
      }
    }
    fsync(fileno(fn));
    fclose(fn);

    const int NameSize=128;
    char oname1[NameSize];
    strncpy(oname1,oname.c_str(),NameSize);
    int fd = open(dirname(oname1),O_RDONLY|O_DIRECTORY);
    if (fd>=0) {
      fsync(fd);
      close(fd);
    }
  }

  if (fail)
    rename(nname.c_str(),oname.c_str());
    
  return msg;
}
