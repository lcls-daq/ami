#include "CspadCalib.hh"

#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"

#include "pdsdata/cspad/Detector.hh"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <libgen.h>

using namespace Ami;

static const unsigned _option_no_pedestal         = 0x01;
static const unsigned _option_reload_pedestal     = 0x02;
static const unsigned _option_correct_common_mode = 0x04;
static const unsigned _option_suppress_bad_pixels = 0x08;
static const unsigned _option_post_integral       = 0x10;

unsigned CspadCalib::option_no_pedestal        () { return _option_no_pedestal; }
unsigned CspadCalib::option_reload_pedestal    () { return _option_reload_pedestal; }
unsigned CspadCalib::option_correct_common_mode() { return _option_correct_common_mode; }
unsigned CspadCalib::option_suppress_bad_pixels() { return _option_suppress_bad_pixels; }
unsigned CspadCalib::option_post_integral      () { return _option_post_integral; }

std::string CspadCalib::save_pedestals(Entry* e,
                                       bool   subtract,
                                       bool   prod)
{
  std::string msg;

  const Ami::EntryImage& entry = *static_cast<const Ami::EntryImage*>(e);
  const Ami::DescImage& desc = entry.desc();
  const unsigned nframes = entry.desc().nframes();

  //
  //  Load pedestals
  //
  double** _off = new double*[nframes];
  for(unsigned s=0; s<nframes; s++) {
    _off[s] = new double[Pds::CsPad::MaxRowsPerASIC*Pds::CsPad::ColumnsPerASIC*2];
    memset(_off[s],0,Pds::CsPad::MaxRowsPerASIC*Pds::CsPad::ColumnsPerASIC*2*sizeof(double));
  }

  if (subtract) {

    //
    //  Load pedestals
    //
    const int NameSize=128;
    char oname1[NameSize];
    char oname2[NameSize];
    
    sprintf(oname1,"ped.%08x.dat",desc.info().phy());
    sprintf(oname2,"/reg/g/pcds/pds/cspadcalib/ped.%08x.dat",desc.info().phy());
    FILE *f = Calib::fopen_dual(oname1, oname2, "pedestals");
    
    if (f) {

      //  read pedestals
      size_t sz = 8 * 1024;
      char* linep = (char *)malloc(sz);
      char* pEnd;
      
      for(unsigned s=0; s<nframes; s++) {
        double* off = _off[s];
        for(unsigned col=0; col<Pds::CsPad::ColumnsPerASIC; col++) {
          getline(&linep, &sz, f);
          *off++ = strtod(linep,&pEnd);
          for (unsigned row=1; row < 2*Pds::CsPad::MaxRowsPerASIC; row++)
            *off++ = strtod(pEnd, &pEnd);
        }
      }    
      
      free(linep);
      fclose(f);
    }
  }

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
  else if ((entry.desc().info().device()==Pds::DetInfo::Cspad2x2 && nframes != 2) ||
           (entry.desc().info().device()==Pds::DetInfo::Cspad    && nframes != 32)) {
    msg = std::string("Failed.  Must readout entire detector.  Check configuration.");
    fail=true;
    fclose(fn);
  }
  else {
    unsigned ppx = entry.desc().ppxbin();
    unsigned ppy = entry.desc().ppybin();
    for(unsigned i=0; i<nframes; i++) {
      double* off = _off[i];
      const SubFrame& frame = entry.desc().frame(i);
      double dn   = entry.info(EntryImage::Normalization)*double(ppx*ppy);
      double doff = entry.info(EntryImage::Pedestal);

      enum Rotation { D0, D90, D180, D270, NPHI=4 };
      static const Rotation _tr[] = {  D0  , D90 , D180, D90 ,
                                       D90 , D180, D270, D180,
                                       D180, D270, D0  , D270,
                                       D270, D0  , D90 , D0 };
      
      switch(_tr[i>>1]) {
      case D0:
        { unsigned x = frame.x;
          unsigned y = frame.y + frame.ny - 1;
          for(unsigned col=0; col<Pds::CsPad::ColumnsPerASIC; col++) {
            for (unsigned row=0; row < 2*Pds::CsPad::MaxRowsPerASIC; row++,off++)
              fprintf(fn, " %.0f", double(*off) + (double(entry.content(x+col/ppx,y-row/ppy))-doff)/dn);
            fprintf(fn, "\n");
          }
          break; }
      case D90:
        { unsigned x = frame.x;
          unsigned y = frame.y;
          for(unsigned col=0; col<Pds::CsPad::ColumnsPerASIC; col++) {
            for (unsigned row=0; row < 2*Pds::CsPad::MaxRowsPerASIC; row++,off++)
              fprintf(fn, " %.0f", double(*off) + (double(entry.content(x+row/ppx,y+col/ppy))-doff)/dn);
            fprintf(fn, "\n");
          }
          break; }
      case D180:
        { unsigned x = frame.x + frame.nx - 1;
          unsigned y = frame.y;
          for(unsigned col=0; col<Pds::CsPad::ColumnsPerASIC; col++) {
            for (unsigned row=0; row < 2*Pds::CsPad::MaxRowsPerASIC; row++,off++)
              fprintf(fn, " %.0f", double(*off) + (double(entry.content(x-col/ppx,y+row/ppy))-doff)/dn);
            fprintf(fn, "\n");
          }
          break; }
      case D270:
        { unsigned x = frame.x + frame.nx - 1;
          unsigned y = frame.y + frame.ny - 1;
          for(unsigned col=0; col<Pds::CsPad::ColumnsPerASIC; col++) {
            for (unsigned row=0; row < 2*Pds::CsPad::MaxRowsPerASIC; row++,off++)
              fprintf(fn, " %.0f", double(*off) + (double(entry.content(x-row/ppx,y-col/ppy))-doff)/dn);
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
    
  for(unsigned s=0; s<nframes; s++)
    delete _off[s];
  delete[] _off;

  return msg;
}
