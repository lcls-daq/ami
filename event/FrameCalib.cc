#include "FrameCalib.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <libgen.h>
#include <math.h>

//#define DBUG

using namespace Ami;

static const unsigned _option_no_pedestal         = 0x01;
static const unsigned _option_reload_pedestal     = 0x02;
static const unsigned _option_correct_common_mode = 0x04;
static const unsigned _option_correct_common_mode2= 0x08;
static const unsigned _option_correct_common_mode3= 0x10;
static const unsigned _option_correct_gain        = 0x20;
static const unsigned _option_suppress_bad_pixels = 0x40;

unsigned FrameCalib::option_no_pedestal        () { return _option_no_pedestal; }
unsigned FrameCalib::option_reload_pedestal    () { return _option_reload_pedestal; }
unsigned FrameCalib::option_correct_common_mode() { return _option_correct_common_mode; }
unsigned FrameCalib::option_suppress_bad_pixels () { return _option_suppress_bad_pixels; }
unsigned FrameCalib::option_correct_common_mode2() { return _option_correct_common_mode2; }
unsigned FrameCalib::option_correct_common_mode3() { return _option_correct_common_mode3; }
unsigned FrameCalib::option_correct_gain        () { return _option_correct_gain; }

static void copyfile(const char* oname,
                     const char* nname)
{
  FILE* fi = fopen(oname,"r");
  if (!fi) return;

  FILE* fo = fopen(nname,"w");
  if (!fo)
    printf("Failed to copy %s to %s\n",oname,nname);
  else {
    unsigned maxln=256*1024;
    char line[maxln];
    while(!feof(fi)) {
      size_t nr = fread (line, 1, maxln, fi);
      if (nr==0) break;
      size_t nw = fwrite(line, 1, nr, fo);
      if (nr!=nw)
        printf("Error writing [%zd/%zd] bytes\n",nw,nr);
    }
    fclose(fo);
  }
  fclose(fi);
}

std::string FrameCalib::save_pedestals(Entry* e,
                                       bool   subtract,
                                       bool   prod,
				       const char* prefix)
{
  std::string msg;

  const Ami::EntryImage& entry = *static_cast<const Ami::EntryImage*>(e);
  const Ami::DescImage& desc = entry.desc();
  const unsigned nframes = entry.desc().nframes();

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
    
    sprintf(oname1,"%s.%08x.dat",prefix,desc.info().phy());
    sprintf(oname2,"/cds/group/pcds/pds/framecalib/%s.%08x.dat",prefix,desc.info().phy());
    FILE *f = Calib::fopen_dual(oname1, oname2, "pedestals");
    
    if (f) {

      CalibIO fio(*f);

      if (nframes) {
	for(unsigned s=0; s<nframes; s++) {
	  const SubFrame& fr = desc.frame(s);
	  unsigned* off = _off[s];
	  for (int row=0; row < fr.ny*desc.ppybin(); row++)
            if (fio.next_line())
              for(int col=0; col<fr.nx*desc.ppxbin(); col++) 
                *off++ = fio.getul();
	}    
      }
      else {
	unsigned* off = _off[0];
	for (unsigned row=0; row < desc.nbinsy()*desc.ppybin(); row++)
          if (fio.next_line())
            for(unsigned col=0; col<desc.nbinsx()*desc.ppxbin(); col++) 
              *off++ = fio.getul();
      }

      fclose(f);
    }
  }

  char tbuf[64];
  sprintf(tbuf,"%s.%08x.dat",prefix,desc.info().phy());
  std::string oname;

  if (prod)
    oname = std::string("/cds/group/pcds/pds/framecalib/") + tbuf;
  else
    oname = std::string("./") + tbuf;

  //  rename current pedestal file
  time_t t = time(0);
  strftime(tbuf,32,"%Y%m%d_%H%M%S",localtime(&t));
    
  std::string nname = oname + "." + tbuf;
  copyfile(oname.c_str(),nname.c_str());

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
	for (unsigned row=0; row < frame.ny*ppy; row++) {
	  for(unsigned col=0; col<frame.nx*ppx; col++,off++)
	    fprintf(fn, " %d", *off + int((double(entry.content(x+col/ppx,y+row/ppy))-doff)/dn+0.5));
	  fprintf(fn, "\n");
	}
      }
    }
    else {
      unsigned* off = _off[0];
      unsigned x = 0;
      unsigned y = 0;
      for (unsigned row=0; row < desc.nbinsy()*ppy; row++) {
	for(unsigned col=0; col<desc.nbinsx()*ppx; col++,off++)
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
    copyfile(nname.c_str(),oname.c_str());
    
  for(unsigned s=0; s<nframes || s<1; s++)
    delete _off[s];

  return msg;
}

bool FrameCalib::load_pedestals(EntryImage* c,
                                unsigned    offset,
                                const char* prefix)
{
  //
  //  Load calibration from a file
  //    Always read and write values for each pixel (even when binned)
  //
  const int NameSize=128;
  char oname1[NameSize];
  char oname2[NameSize];
  const DescImage& d = c->desc();
  sprintf(oname1,"%s.%08x.dat",prefix,d.info().phy());
  sprintf(oname2,"/cds/group/pcds/pds/framecalib/%s",oname1);
  FILE* f = Calib::fopen_dual(oname1,oname2,"pedestals");
  if (f) {
    bool result=load_pedestals(c,offset,f);
    fclose(f);
    return result;
  }
  else
    return false;
}

bool  FrameCalib::load_pedestals_all(EntryImage* c,
                                     unsigned offset,
                                     const char* onl_prefix,
                                     const char* off_prefix)
{
  //
  //  Load calibration from a file (include offline too)
  //    Always read and write values for each pixel (even when binned)
  //
  const DescImage& d = c->desc();
  FILE* f = Calib::fopen(d.info(), onl_prefix, off_prefix);
  if (f) {
    bool result=load_pedestals(c,offset,f);
    fclose(f);
    return result;
  }
  else
    return false;
}

bool FrameCalib::load_pedestals(EntryImage* c,
                                unsigned offset,
                                FILE* f)
{
  CalibIO fio(*f);

  const DescImage& d = c->desc();
  const unsigned ppb  = d.ppxbin();

  c->reset();

  if (d.nframes()) {
    for(unsigned i=0; i<d.nframes(); i++) {
      const SubFrame& frame = d.frame(i);

      unsigned x = frame.x;
      unsigned y = frame.y;
      for(unsigned row=0; row < frame.ny*ppb; row++)
        if (fio.next_line()) {
          unsigned col=0;
          while(1) {
            int v = int(fio.getdb());
            if (fio.get_failed()) {
              // Aren't enough cols in the data
              printf("FrameCalib[%s] retrieved pedestal has [%d] columns which is smaller than the expected [%d]!\n",
                     Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(d.info())),
                     col,
                     frame.nx*ppb);
              c->reset();
              return false;
            }
            c->addcontent(offset-v,x+col/ppb,y+row/ppb);
            if (++col >= frame.nx*ppb) break;
          }
          // test if there are more columns in this row than expected
          fio.getdb();
          if (!fio.get_failed()) {
            printf("FrameCalib[%s] retrieved pedestal has more columns than the expected [%d]\n",
                   Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(d.info())),
                   frame.nx*ppb);
            c->reset();
            return false;
          }
        } else {
          // Aren't enough rows in the data
          printf("FrameCalib[%s] retrieved pedestal has only %d rows smaller than the expected %d\n",
                 Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(d.info())),
                 row,
                 frame.ny*ppb);
          c->reset();
          return false;
        }
      // test if there are more rows in the data than expected
      if (fio.next_line()) {
        printf("FrameCalib[%s] retrieved pedestal has more rows than the expected %d\n",
               Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(d.info())),
               frame.ny*ppb);
        c->reset();
        return false;
      }
    }
  }
  else {
    for(unsigned row=0; row < d.nbinsy()*ppb; row++)
      if (fio.next_line()) {
        unsigned col=0;
        while(1) {
          int v = int(fio.getdb());
          if (fio.get_failed()) {
            // Aren't enough cols in the data
            printf("FrameCalib[%s] retrieved pedestal has %d columns smaller than the expected %d\n",
                   Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(d.info())),
                   col,
                   d.nbinsx()*ppb);
            c->reset();
            return false;
          }
          c->addcontent(offset-v,col/ppb,row/ppb);
          if (++col >= d.nbinsx()*ppb) break;
        }
        // test if there are more columns in this row than expected
        fio.getdb();
        if (!fio.get_failed()) {
          printf("FrameCalib[%s] retrieved pedestal has more columns than the expected %d\n",
                 Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(d.info())),
                 d.nbinsx()*ppb);
          c->reset();
          return false;
        }
      } else {
        // Aren't enough rows in the data
        printf("FrameCalib[%s] retrieved pedestal has only %d rows smaller than the expected %d\n",
               Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(d.info())),
               row,
               d.nbinsy()*ppb);
        c->reset();
        return false;  
      }
    // test if there are more rows in the data than expected
    if (fio.next_line()) {
      printf("FrameCalib[%s] retrieved pedestal has more rows than the expected %d\n",
             Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(d.info())),
             d.nbinsy()*ppb);
      c->reset();
      return false;
    }
  }
  return true;
}

std::string FrameCalib::save(ndarray<const double,3> a,
			     const DescImage&        desc,
			     bool                    prod,
			     const char*             prefix)
{
  std::string msg;

  const unsigned nframes = desc.nframes();

  char tbuf[64];
  sprintf(tbuf,"%s.%08x.dat",prefix,desc.info().phy());
  std::string oname;

  if (prod)
    oname = std::string("/cds/group/pcds/pds/framecalib/") + tbuf;
  else
    oname = std::string("./") + tbuf;

  //  rename current file
  time_t t = time(0);
  strftime(tbuf,32,"%Y%m%d_%H%M%S",localtime(&t));
    
  std::string nname = oname + "." + tbuf;
  copyfile(oname.c_str(),nname.c_str());

  bool fail=false;
  FILE* fn = fopen(oname.c_str(),"w");
  if (!fn) {
    msg = std::string("Unable to write new file ") + oname;
    fail=true;
  }
  else {
    const unsigned ppx = desc.ppxbin();
    const unsigned ppy = desc.ppybin();

    if (nframes) {
      for(unsigned i=0; i<nframes; i++) {
	const SubFrame& frame = desc.frame(i);
	for (unsigned row=0; row < frame.ny*ppy; row++) {
	  for(unsigned col=0; col<frame.nx*ppx; col++)
	    fprintf(fn, " %f", a(i,row,col));
	  fprintf(fn, "\n");
	}
      }
    }
    else {
      for (unsigned row=0; row < desc.nbinsy()*ppy; row++) {
	for(unsigned col=0; col<desc.nbinsx()*ppx; col++)
	  fprintf(fn, " %f", a(0,row,col));
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
    copyfile(nname.c_str(),oname.c_str());
    
  return msg;
}

ndarray<double,3> FrameCalib::load(const DescImage& d,
				   const char*      prefix)
{
  ndarray<double,3> a;

  //
  //  Load calibration from a file
  //    Always read and write values for each pixel (even when binned)
  //
  const int NameSize=128;
  char oname1[NameSize];
  char oname2[NameSize];
  sprintf(oname1,"%s.%08x.dat",prefix,d.info().phy());
  sprintf(oname2,"/cds/group/pcds/pds/framecalib/%s",oname1);
  FILE* f = Calib::fopen_dual(oname1,oname2,prefix);
  if (f) {
    a = load(d,f);
    fclose(f);
  }
  else
    a = make_ndarray<double>(0U,0,0);
  return a;
}

ndarray<double,3> FrameCalib::load(const DescImage& d,
                                   FILE*            f)
{
  ndarray<double,3> a = d.nframes() ?
    make_ndarray<double>(d.nframes(),d.frame(0).ny,d.frame(0).nx) :
    make_ndarray<double>(1,d.nbinsy(),d.nbinsx());

  CalibIO fio(*f);

  const unsigned ppb  = d.ppxbin();

  if (d.nframes()) {
    for(unsigned i=0; i<d.nframes(); i++) {
      const SubFrame& frame = d.frame(i);
      for(unsigned row=0; row < frame.ny*ppb; row++)
        if (fio.next_line()) {
          char* p = fio.line();
          unsigned col=0;
          while(1) {
            char* pEnd;
            double v = strtod(p,&pEnd);
            if (pEnd == p) break;
            //  when binned, simply take last read value for that bin
            a(i,row/ppb,col/ppb) = v;
            col++;
            p = pEnd+1;
          }
        }
    }
  }
  else {
    for(unsigned row=0; row < d.nbinsy()*ppb; row++)
      if (fio.next_line()) {
        char* p = fio.line();
        unsigned col=0;
        while(1) {
          char* pEnd;
          double v = strtod(p,&pEnd);
          if (pEnd == p) break;
          a(0,row/ppb,col/ppb) = v;
          col++;
          p = pEnd+1;
        }
      }
  }
  
  return a;
}

ndarray<double,2> FrameCalib::load_darray(const DescImage& d,
					  const char*      prefix)
{
  unsigned max_rows    = d.nbinsy();
  unsigned max_columns = d.nbinsx();
  ndarray<double,2> a = 
    make_ndarray<double>(max_rows, max_columns);

  //
  //  Load calibration from a file
  //    Always read and write values for each pixel (even when binned)
  //
  const int NameSize=128;
  char oname1[NameSize];
  char oname2[NameSize];
  sprintf(oname1,"%s.%08x.dat",prefix,d.info().phy());
  sprintf(oname2,"/cds/group/pcds/pds/framecalib/%s",oname1);
  FILE* f = Calib::fopen_dual(oname1,oname2,prefix);
  if (f) {
    CalibIO fio(*f);

    const unsigned ppb  = d.ppxbin();

    unsigned maxc = 0;
    unsigned row = 0;
    do {
      if (!fio.next_line()) break;
      char* p = fio.line();
      unsigned col=0;
      while(col < max_columns) {
        char* pEnd;
	double v = strtod(p,&pEnd);
	if (pEnd == p) break;
	a(row/ppb,col/ppb) = v;
	col++;
	p = pEnd+1;
      }
      if (col > maxc) maxc=col;
    } while (++row < max_rows);
    
    fclose(f);

    if (maxc != max_columns ||
	row  != max_rows) {
      ndarray<double,2> b = make_ndarray<double>(row,maxc);
      for(unsigned i=0; i<row; i++) {
	double* p = &b(i,0);
	for(unsigned j=0; j<maxc; j++)
	  p[j] = a(i,j);
      }
      a = b;
    }
  }
  else
    a = make_ndarray<double>(0U,0);
  
  return a;
}

ndarray<unsigned,2> FrameCalib::load_array(const DescImage& d,
					   const char*      prefix)
{
  unsigned max_rows    = d.nbinsy();
  unsigned max_columns = d.nbinsx();
  ndarray<unsigned,2> a = 
    make_ndarray<unsigned>(max_rows, max_columns);

  //
  //  Load calibration from a file
  //    Always read and write values for each pixel (even when binned)
  //
  const int NameSize=128;
  char oname1[NameSize];
  char oname2[NameSize];
  sprintf(oname1,"%s.%08x.dat",prefix,d.info().phy());
  sprintf(oname2,"/cds/group/pcds/pds/framecalib/%s",oname1);
  FILE* f = Calib::fopen_dual(oname1,oname2,prefix);
  if (f) {
    CalibIO fio(*f);

    const unsigned ppb  = d.ppxbin();

    unsigned maxc = 0;
    unsigned row = 0;
    do {
      if (!fio.next_line()) break;
      char* p = fio.line();
      unsigned col=0;
      while(col < max_columns) {
        char* pEnd;
	unsigned v = strtoul(p,&pEnd,0);
	if (pEnd == p) break;
	a(row/ppb,col/ppb) = v;
	col++;
	p = pEnd+1;
      }
      if (col > maxc) maxc=col;
    } while (++row < max_rows);
    
    fclose(f);

    if (maxc != max_columns ||
	row  != max_rows) {
      ndarray<unsigned,2> b = make_ndarray<unsigned>(row,maxc);
      for(unsigned i=0; i<row; i++) {
	unsigned* p = &b(i,0);
	for(unsigned j=0; j<maxc; j++)
	  p[j] = a(i,j);
      }
      a = b;
    }
  }
  else
    a = make_ndarray<unsigned>(0U,0);
  
  return a;
}

ndarray<double,2> FrameCalib::load_darray(const DescImage& d,
                                          const char*      prefix,
                                          const char*      offl_type)
{
  unsigned max_rows    = d.nbinsy();
  unsigned max_columns = d.nbinsx();
  ndarray<double,2> a = 
    make_ndarray<double>(max_rows, max_columns);

  //
  //  Load calibration from a file
  //    Always read and write values for each pixel (even when binned)
  //
  FILE* f = Calib::fopen(d.info(), prefix, offl_type);
  if (f) {
    CalibIO fio(*f);

    const unsigned ppb  = d.ppxbin();

    unsigned maxc = 0;
    unsigned row = 0;
    do {
      if (!fio.next_line()) break;
      char* p = fio.line();
      unsigned col=0;
      while(col < max_columns) {
        char* pEnd;
	double v = strtod(p,&pEnd);
	if (pEnd == p) break;
	a(row/ppb,col/ppb) = v;
	col++;
	p = pEnd+1;
      }
      if (col > maxc) maxc=col;
    } while (++row < max_rows);
    
    fclose(f);

    if (maxc != max_columns ||
	row  != max_rows) {
      ndarray<double,2> b = make_ndarray<double>(row/ppb,maxc/ppb);
      for(unsigned i=0; i<row/ppb; i++) {
	double* p = &b(i,0);
	for(unsigned j=0; j<maxc/ppb; j++)
	  p[j] = a(i,j);
      }
      a = b;
    }
  }
  else
    a = make_ndarray<double>(0U,0);
  
  return a;
}

ndarray<unsigned,2> FrameCalib::load_array(FILE* f)
{
  unsigned ncols=0;
  unsigned nrows=1;
  { CalibIO fio(*f);

    if (!fio.next_line())
      return make_ndarray<unsigned>(0U,0);

    while(1) {
      fio.getul();
      if (fio.get_failed()) break;
      ncols++;
    }

    while(fio.next_line()) {
      nrows++;
    }
  }

  ndarray<unsigned,2> a = make_ndarray<unsigned>(nrows, ncols);
  rewind(f);
  
  CalibIO fio(*f);
  for(unsigned i=0; i<nrows; i++) {
    fio.next_line();
    for(unsigned j=0; j<ncols; j++)
      a(i,j) = fio.getul();
  }

  return a;
}

ndarray<double,2> FrameCalib::load_darray(FILE* f)
{
  unsigned ncols=0;
  unsigned nrows=1;
  { CalibIO fio(*f);

    if (!fio.next_line())
      return make_ndarray<double>(0U,0);

    while(1) {
      fio.getdb();
      if (fio.get_failed()) break;
      ncols++;
    }

    while(fio.next_line()) {
      nrows++;
    }
  }

  ndarray<double,2> a = make_ndarray<double>(nrows, ncols);
  rewind(f);
  
  CalibIO fio(*f);
  for(unsigned i=0; i<nrows; i++) {
    fio.next_line();
    for(unsigned j=0; j<ncols; j++)
      a(i,j) = fio.getdb();
  }

  return a;
}

int FrameCalib::median(ndarray<const uint16_t,1> data,
		       unsigned& iLo, unsigned& iHi)
{
  unsigned* bins = 0;
  unsigned nbins = 0;

  while(1) {
    if (bins) delete[] bins;

    if (iLo>iHi) {
      printf("Warning: FrameCalib::median iLo,iHi arguments reversed [%d,%d]\n",
             iLo,iHi);
      unsigned v=iLo; iLo=iHi; iHi=v;
    }

    nbins = iHi-iLo+1;

    if (nbins>10000) {
      printf("Warning: FrameCalib::median too many bins [%d]\n",nbins);
      return -1;
    }

    bins  = new unsigned[nbins];
    memset(bins,0,nbins*sizeof(unsigned));

    for(unsigned i=0; i<data.size(); i++) {
      if (data[i] < iLo)
	bins[0]++;
      else if (data[i] >= iHi)
	bins[nbins-1]++;
      else
	bins[data[i]-iLo]++;
    }
    
    if (bins[0] > data.size()/2)
      if (iLo > nbins/4)
	iLo -= nbins/4;
      else if (iLo > 0)
	iLo = 0;
      else {
	delete[] bins;
	return iLo;
      }
    else if (bins[nbins-1] > data.size()/2)
      iHi += nbins/4;
    else
      break;
  }
    
  unsigned i=1;
  int s=data.size()/2-bins[0];
  while( s>0 )
    s -= bins[i++];

  i--;
  //  if (unsigned(-s) > bins[i]/2) i--;

  delete[] bins;

  return (iLo+i);
}

int FrameCalib::median(ndarray<const uint16_t,1> data,
                       ndarray<const uint16_t,1> status,
		       unsigned& iLo, unsigned& iHi)
{
  unsigned* bins = 0;
  unsigned nbins = 0;
  unsigned dsize = 0;

  while(1) {
    if (bins) delete[] bins;

    if (iLo>iHi) {
      printf("Warning: FrameCalib::median iLo,iHi arguments reversed [%d,%d]\n",
             iLo,iHi);
      unsigned v=iLo; iLo=iHi; iHi=v;
    }

    nbins = iHi-iLo+1;

    if (nbins>10000) {
      printf("Warning: FrameCalib::median too many bins [%d]\n",nbins);
      return -1;
    }

    bins  = new unsigned[nbins];
    memset(bins,0,nbins*sizeof(unsigned));

    dsize=0;
    for(unsigned i=0; i<data.size(); i++) {
      if (status[i] > 0)
        continue;
      
      dsize++;
      if (data[i] < iLo)
	bins[0]++;
      else if (data[i] >= iHi)
	bins[nbins-1]++;
      else
	bins[data[i]-iLo]++;
    }
    
    if (bins[0] > dsize/2)
      if (iLo > nbins/4)
	iLo -= nbins/4;
      else if (iLo > 0)
	iLo = 0;
      else {
	delete[] bins;
	return iLo;
      }
    else if (bins[nbins-1] > dsize/2)
      iHi += nbins/4;
    else
      break;
  }
    
  unsigned i=1;
  int s=dsize/2-bins[0];
  while( s>0 )
    s -= bins[i++];

  i--;
  //  if (unsigned(-s) > bins[i]/2) i--;

  delete[] bins;

  return (iLo+i);
}

int FrameCalib::median(ndarray<const uint32_t,1> data,
		       unsigned& iLo, unsigned& iHi)
{
  unsigned* bins = 0;
  unsigned nbins = 0;

  while(1) {
    if (bins) delete[] bins;

    if (iLo>iHi) {
      printf("Warning: FrameCalib::median iLo,iHi arguments reversed [%d,%d]\n",
             iLo,iHi);
      unsigned v=iLo; iLo=iHi; iHi=v;
    }

    nbins = iHi-iLo+1;

    if (nbins>10000) {
      printf("Warning: FrameCalib::median too many bins [%d]\n",nbins);
      return -1;
    }

    bins  = new unsigned[nbins];
    memset(bins,0,nbins*sizeof(unsigned));

    for(unsigned i=0; i<data.size(); i++) {
      if (data[i] < iLo)
	bins[0]++;
      else if (data[i] >= iHi)
	bins[nbins-1]++;
      else
	bins[data[i]-iLo]++;
    }
    
    if (bins[0] > data.size()/2)
      if (iLo > nbins/4)
	iLo -= nbins/4;
      else if (iLo > 0)
	iLo = 0;
      else {
	delete[] bins;
	return iLo;
      }
    else if (bins[nbins-1] > data.size()/2)
      iHi += nbins/4;
    else
      break;
  }
    
  unsigned i=1;
  int s=data.size()/2-bins[0];
  while( s>0 )
    s -= bins[i++];

  i--;
  //  if (unsigned(-s) > bins[i]/2) i--;

  delete[] bins;

#ifdef DBUG
    double x0=data.size(), x1=0, x2=0;
    for(unsigned i=0; i<data.size(); i++) {
      double x = double(data[i]);
      x1 += x;
      x2 += x*x;
    }
    printf("::median stat %f %f [%f]\n",
	   x1/x0,sqrt(x2/x0-pow(x1/x0,2)),
	   double(iLo+i)-x1/x0);
#endif
  
  return (iLo+i);
}

int FrameCalib::median(ndarray<const uint32_t,1> data,
                       ndarray<const uint32_t,1> status,
		       unsigned& iLo, unsigned& iHi)
{
  unsigned* bins = 0;
  unsigned nbins = 0;
  unsigned dsize = 0;

  while(1) {
    if (bins) delete[] bins;

    if (iLo>iHi) {
      printf("Warning: FrameCalib::median iLo,iHi arguments reversed [%d,%d]\n",
             iLo,iHi);
      unsigned v=iLo; iLo=iHi; iHi=v;
    }

    nbins = iHi-iLo+1;

    if (nbins>10000) {
      printf("Warning: FrameCalib::median too many bins [%d]\n",nbins);
      return -1;
    }

    bins  = new unsigned[nbins];
    memset(bins,0,nbins*sizeof(unsigned));

    dsize=0;
    for(unsigned i=0; i<data.size(); i++) {
      if (status[i] > 0)
        continue;

      dsize++;
      if (data[i] < iLo)
	bins[0]++;
      else if (data[i] >= iHi)
	bins[nbins-1]++;
      else
	bins[data[i]-iLo]++;
    }
    
    if (bins[0] > dsize/2)
      if (iLo > nbins/4)
	iLo -= nbins/4;
      else if (iLo > 0)
	iLo = 0;
      else {
	delete[] bins;
	return iLo;
      }
    else if (bins[nbins-1] > dsize/2)
      iHi += nbins/4;
    else
      break;
  }
    
  unsigned i=1;
  int s=dsize/2-bins[0];
  while( s>0 )
    s -= bins[i++];

  i--;
  //  if (unsigned(-s) > bins[i]/2) i--;

  delete[] bins;

  return (iLo+i);
}

int FrameCalib::median(ndarray<const uint32_t,2> d,
		       unsigned& iLo, unsigned& iHi)
{
  unsigned* bins = 0;
  unsigned nbins = 0;

  while(1) {
    if (bins) delete[] bins;

    if (iLo>iHi) {
      printf("Warning: FrameCalib::median iLo,iHi arguments reversed [%d,%d]\n",
             iLo,iHi);
      unsigned v=iLo; iLo=iHi; iHi=v;
    }

    nbins = iHi-iLo+1;

    if (nbins>10000) {
      printf("Warning: FrameCalib::median too many bins [%d]\n",nbins);
      return -1;
    }

    bins  = new unsigned[nbins];
    memset(bins,0,nbins*sizeof(unsigned));

    for(unsigned j=0; j<d.shape()[0]; j++) {
      const uint32_t* data = &d(j,0);
      for(unsigned i=0; i<d.shape()[1]; i++)
        if (data[i] < iLo)
          bins[0]++;
        else if (data[i] >= iHi)
          bins[nbins-1]++;
        else
          bins[data[i]-iLo]++;
    }
    
    if (bins[0] > d.size()/2)
      if (iLo > nbins/4) 
	iLo -= nbins/4;
      else if (iLo > 0)
	iLo = 0;
      else {
	delete[] bins;
	return iLo;
      }
    else if (bins[nbins-1] > d.size()/2)
      iHi += nbins/4;
    else
      break;
  }
    
  unsigned i=1;
  int s=d.size()/2-bins[0];
  while( s>0 )
    s -= bins[i++];

  i--;
  //  if (-s > int(bins[i]/2)) i--;

  delete[] bins;

#ifdef DBUG
    double x0=0, x1=0, x2=0;
    for(unsigned j=0; j<d.shape()[0]; j++) {
      const uint32_t* data = &d[j][0];
      for(unsigned i=0; i<d.shape()[1]; i++) {
	double x = double(data[i]);
	x0++;
	x1 += x;
	x2 += x*x;
      }
    }
    printf("::median stat %f %f [%f]\n",
	   x1/x0,sqrt(x2/x0-pow(x1/x0,2)),
	   double(iLo+i)-x1/x0);
#endif

  return (iLo+i);
}

int FrameCalib::median(ndarray<const int32_t,1> data,
		       int& iLo, int& iHi,
		       unsigned*& bins)
{
  unsigned nbins = iHi-iLo+1;

  while(1) {
    memset(bins,0,nbins*sizeof(unsigned));

    for(unsigned i=0; i<data.size(); i++) {
      if (data[i] < iLo)
	bins[0]++;
      else if (data[i] >= iHi)
	bins[nbins-1]++;
      else
	bins[data[i]-iLo]++;
    }
    
    if (bins[0] > data.size()/2)
      iLo -= nbins/4;
    else if (bins[nbins-1] > data.size()/2)
      iHi += nbins/4;
    else
      break;

    delete[] bins;
    nbins = iHi-iLo+1;
    bins  = new unsigned[nbins];
  }
    
  unsigned i=1;
  //  int s=(data.size()-bins[0]-bins[nbins-1])/2;
  int s=data.size()/2-bins[0];
  while( s>0 )
    s -= bins[i++];

  i--;
  //  if (-s > int(bins[i]/2)) i--;

  return (iLo+i);
}

int FrameCalib::frameNoise(ndarray<const uint32_t,1> data,
                           ndarray<const uint32_t,1> status,
                           unsigned off)
{
  unsigned lo = off-100;
  unsigned hi = off+100;
  return FrameCalib::median(data,status,lo,hi)-int(off);
}

double FrameCalib::frameNoise(ndarray<const uint32_t,2> data,
                              ndarray<const uint32_t,2> status,
                              unsigned off)
{
  const int fnPixelMin = -100 + off;
  const int fnPixelMax =  100 + off;
  const int fnPixelBins = fnPixelMax - fnPixelMin;
  const int peakSpace   = 5;
  
  //  histogram the pixel values
  unsigned hist[fnPixelBins];
  unsigned nhist=0;
  { memset(hist, 0, fnPixelBins*sizeof(unsigned));
    for(unsigned i=0; i<data.shape()[0]; i++) {
      for(unsigned j=0; j<data.shape()[1]; j++) {
        if (status(i,j)==0) {
          nhist++;
          int v = data(i,j) - fnPixelMin;
          if (v >= 0 && v < int(fnPixelBins))
            hist[v]++;
        }
      }
    }
  }

  double v = 0;
  // the first peak from the left above this is the pedestal
  { const int fnPeakBins = 5;
    const int fnPixelRange = fnPixelBins-fnPeakBins-1;
    const unsigned fnPedestalThreshold = nhist>>5;
    
    unsigned i=fnPeakBins;
    while( int(i)<fnPixelRange ) {
      if (hist[i]>fnPedestalThreshold) break;
      i++;
    }

    unsigned thresholdPeakBin=i;
    unsigned thresholdPeakBinContent=hist[i];
    while( int(++i)<fnPixelRange ) {
      if (hist[i]<thresholdPeakBinContent) {
        if (i > thresholdPeakBin+peakSpace)
          break;
      }
      else {
        thresholdPeakBin = i;
        thresholdPeakBinContent = hist[i];
      }
    }

    i = thresholdPeakBin;
    if ( int(i)+fnPeakBins<=fnPixelRange ) {
      unsigned s0 = 0;
      unsigned s1 = 0;
      for(unsigned j=i-fnPeakBins+1; j<i+fnPeakBins; j++) {
        s0 += hist[j];
        s1 += hist[j]*j;
      }
      
      double binMean = double(s1)/double(s0);
      v =  binMean + fnPixelMin - off;
      
      s0 = 0;
      unsigned s2 = 0;
      for(unsigned j=i-10; j<i+fnPeakBins; j++) {
        s0 += hist[j];
        s2 += hist[j]*(j-int(binMean))*(j-int(binMean));
      }
//      const double allowedPedestalWidthSquared = 2.5*2.5;
      //      printf("frameNoise finds mean %f, variance %f\n", v, double(s2)/double(s0));
//      if (double(s2)/double(s0)>allowedPedestalWidthSquared) v = 0;
      // this isn't the standard rms around the mean, but should be similar if rms_real < 3
      //      printf("frameNoise finds mean %f, variance %f\n", v, double(s2)/double(s0));

    }
    else {
      static unsigned nPrint=0;
      nPrint++;
      if ((nPrint<10) || (nPrint%100)==0)
        printf("frameNoise : peak not found [%d]\n",nPrint);
    }
//    printf("CspadMiniHandler::frameNoise v=%lf\n", v);
  }

  return v;
}

