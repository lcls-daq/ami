#include "FrameCalib.hh"

#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"

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

unsigned FrameCalib::option_no_pedestal        () { return _option_no_pedestal; }
unsigned FrameCalib::option_reload_pedestal    () { return _option_reload_pedestal; }
unsigned FrameCalib::option_correct_common_mode() { return _option_correct_common_mode; }

std::string FrameCalib::save_pedestals(Entry* e,
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
  unsigned* _off[nframes ? nframes : 1];
  if (nframes) {
    for(unsigned s=0; s<nframes; s++) {
      const SubFrame& f = desc.frame(s);
      unsigned sz = f.nx*desc.ppxbin()*f.ny*desc.ppybin();
      _off[s] = new unsigned[sz];
      memset(_off[s],0,sz*sizeof(unsigned));
    }
  }
  else {
      unsigned sz = desc.nbinsx()*desc.ppxbin()*desc.nbinsy()*desc.ppybin();
      _off[0] = new unsigned[sz];
      memset(_off[0],0,sz*sizeof(unsigned));
  }

  if (subtract) {

    //
    //  Load pedestals
    //
    const int NameSize=128;
    char oname1[NameSize];
    char oname2[NameSize];
    
    sprintf(oname1,"ped.%08x.dat",desc.info().phy());
    sprintf(oname2,"/reg/g/pcds/pds/framecalib/ped.%08x.dat",desc.info().phy());
    FILE *f = Calib::fopen_dual(oname1, oname2, "pedestals");
    
    if (f) {

      //  read pedestals
      size_t sz = 8 * 1024;
      char* linep = (char *)malloc(sz);
      char* pEnd;

      if (nframes) {
	for(unsigned s=0; s<nframes; s++) {
	  const SubFrame& fr = desc.frame(s);
	  unsigned* off = _off[s];
	  for (unsigned row=0; row < fr.ny*desc.ppybin(); row++) {
	    getline(&linep, &sz, f);
	    *off++ = strtoul(linep,&pEnd,0);
	    for(unsigned col=1; col<fr.nx*desc.ppxbin(); col++) 
	      *off++ = strtoul(pEnd, &pEnd,0);
	  }
	}    
      }
      else {
	unsigned* off = _off[0];
	for (unsigned row=0; row < desc.nbinsy()*desc.ppybin(); row++) {
	  getline(&linep, &sz, f);
	  *off++ = strtoul(linep,&pEnd,0);
	  for(unsigned col=1; col<desc.nbinsx()*desc.ppxbin(); col++) 
	    *off++ = strtoul(pEnd, &pEnd,0);
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
    oname = std::string("/reg/g/pcds/pds/framecalib/ped.") + tbuf;
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
  else {
    const unsigned ppx = entry.desc().ppxbin();
    const unsigned ppy = entry.desc().ppybin();
    const double dn    = entry.info(EntryImage::Normalization)*double(ppx*ppy);
    const double doff  = entry.info(EntryImage::Pedestal);

#ifdef DBUG
    {  // dump the first pixel's inputs, output
      unsigned ix = nframes ? desc.frame(0).x : 0;
      unsigned iy = nframes ? desc.frame(0).y : 0;
      printf("FrameCalib::save_pedestals:  norm %f  doff %f  off[0] %d  content[0] %u  ped[0] %f\n",
             dn, doff,
             *_off[0], entry.content(ix,iy),
             double(*_off[0]) + (double(entry.content(ix,iy)-doff)/dn));
    }
#endif

    if (nframes) {
      for(unsigned i=0; i<nframes; i++) {
	unsigned* off = _off[i];
	const SubFrame& frame = entry.desc().frame(i);

        unsigned x = frame.x;
	unsigned y = frame.y;
	for (unsigned row=0; row < frame.ny*ppy; row++,off++) {
	  for(unsigned col=0; col<frame.nx*ppx; col++)
	    fprintf(fn, " %d", *off + int((double(entry.content(x+col/ppx,y+row/ppy))-doff)/dn+0.5));
	  fprintf(fn, "\n");
	}
      }
    }
    else {
      unsigned* off = _off[0];
      unsigned x = 0;
      unsigned y = 0;
      for (unsigned row=0; row < desc.nbinsy()*ppy; row++,off++) {
	for(unsigned col=0; col<desc.nbinsx()*ppx; col++)
	  fprintf(fn, " %d", *off + int((double(entry.content(x+col/ppx,y+row/ppy))-doff)/dn+0.5));
	fprintf(fn, "\n");
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
    
  for(unsigned s=0; s<nframes || s<1; s++)
    delete _off[s];

  return msg;
}