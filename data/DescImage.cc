#include "ami/data/DescImage.hh"

#include "ami/data/ImageMask.hh"
#include "ami/data/valgnd.hh"

#include <string.h>
#include <stdio.h>

using namespace Ami;

DescImage::DescImage(const char* name, 
                     unsigned nbinsx, 
                     unsigned nbinsy, 
                     int ppbx,
                     int ppby,
                     unsigned xp0,
                     unsigned yp0,
                     bool isnormalized) :
  DescEntry(name, "x", "y", Image, sizeof(DescImage), DescEntry::Mean, isnormalized),
  _nbinsx (nbinsx ? nbinsx : 1),
  _nbinsy (nbinsy ? nbinsy : 1),
  _ppbx   (ppbx),
  _ppby   (ppby),
  _dpbx   (1),
  _dpby   (1),
  _xp0    (xp0),
  _yp0    (yp0),
  _unitppx(0),
  _unitppy(1),
  _reserved  (0),
  _nsubframes(0)
{
  memset(&_subframes[0], 0, MAX_SUBFRAMES*sizeof(SubFrame));
  memset(_mask_path,0,PATHLEN);
  _mask._field = 0;
  strncpy_val(_units, "mms", UnitsSize);
  _units[UnitsSize-1] = 0;
}

DescImage::DescImage(const Pds::DetInfo& info,
                     unsigned channel,
                     const char* name, 
                     unsigned nbinsx, 
                     unsigned nbinsy, 
                     int ppbx,
                     int ppby,
                     int dpbx,
                     int dpby,
                     unsigned xp0,
                     unsigned yp0) :
  DescEntry(info, channel, name, "x", "y", Image, sizeof(DescImage)),
  _nbinsx (nbinsx ? nbinsx : 1),
  _nbinsy (nbinsy ? nbinsy : 1),
  _ppbx   (ppbx),
  _ppby   (ppby),
  _dpbx   (dpbx),
  _dpby   (dpby),
  _xp0    (xp0),
  _yp0    (yp0),
  _unitppx(0),
  _unitppy(1),
  _reserved  (0),
  _nsubframes(0)
{
  memset(&_subframes[0], 0, MAX_SUBFRAMES*sizeof(SubFrame));
  memset(_mask_path,0,PATHLEN);
  _mask._field = 0;
  strncpy_val(_units, "mms", UnitsSize);
  _units[UnitsSize-1] = 0;
}

DescImage::DescImage(const Pds::DetInfo& info,
                     const char* name, 
                     const char* zunits,
                     unsigned nbinsx, 
                     unsigned nbinsy, 
                     int ppbx,
                     int ppby,
                     bool pedCalib,
                     bool gainCalib,
                     bool rmsCalib) :
  DescEntry(info, 0, name, "x", "y", zunits, 
            Image, sizeof(DescImage), DescEntry::Mean,
            true, false, 
            pedCalib, gainCalib, rmsCalib),
  _nbinsx (nbinsx ? nbinsx : 1),
  _nbinsy (nbinsy ? nbinsy : 1),
  _ppbx   (ppbx),
  _ppby   (ppby),
  _dpbx   (1),
  _dpby   (1),
  _xp0    (0),
  _yp0    (0),
  _unitppx(0),
  _unitppy(1),
  _reserved  (0),
  _nsubframes(0)
{
  memset(&_subframes[0], 0, MAX_SUBFRAMES*sizeof(SubFrame));
  memset(_mask_path,0,PATHLEN);
  _mask._field = 0;
  strncpy_val(_units, "mms", UnitsSize);
  _units[UnitsSize-1] = 0;
}

DescImage::DescImage(const DescImage& d) :
  DescEntry(d),
  _nbinsx  (d._nbinsx),
  _nbinsy  (d._nbinsy),
  _ppbx    (d._ppbx),
  _ppby    (d._ppby),
  _dpbx    (d._dpbx),
  _dpby    (d._dpby),
  _xp0     (d._xp0),
  _yp0     (d._yp0),
  _unitppx (d._unitppx),
  _unitppy (d._unitppy),
  _reserved(d._reserved),
  _nsubframes(0)
{
  for(unsigned i=0; i<d._nsubframes; i++) {
    const SubFrame& f = d._subframes[i];
    add_frame(f.x, f.y, f.nx, f.ny, f.r, f.flipx, f.flipy);
  }

  memset(&_subframes[d._nsubframes], 0, (MAX_SUBFRAMES-d._nsubframes)*sizeof(SubFrame));

  strncpy_val(_mask_path, d._mask_path, PATHLEN);

  if (strlen(d._mask_path))
    _mask._ptr = new ImageMask(d._nbinsy, d._nbinsx, d._nsubframes, d._subframes, d._mask_path);
  else if (d.mask())
    _mask._ptr = new ImageMask(*d.mask());
  else
    _mask._field = 0;

  strncpy_val(_units, d._units, UnitsSize);
  _units[UnitsSize-1] = 0;
}

DescImage::DescImage(const DescImage& d, const char* mask_path) :
  DescEntry(d),
  _nbinsx  (d._nbinsx),
  _nbinsy  (d._nbinsy),
  _ppbx    (d._ppbx),
  _ppby    (d._ppby),
  _dpbx    (d._dpbx),
  _dpby    (d._dpby),
  _xp0     (d._xp0),
  _yp0     (d._yp0),
  _unitppx (d._unitppx),
  _unitppy (d._unitppy),
  _reserved(d._reserved),
  _nsubframes(0)
{
  for(unsigned i=0; i<d._nsubframes; i++) {
    const SubFrame& f = d._subframes[i];
    add_frame(f.x, f.y, f.nx, f.ny, f.r, f.flipx, f.flipy);
  }

  memset(&_subframes[d._nsubframes], 0, (MAX_SUBFRAMES-d._nsubframes)*sizeof(SubFrame));

  strncpy_val(_mask_path, mask_path, PATHLEN);

  if (strlen(mask_path))
    _mask._ptr = new ImageMask(d._nbinsy, d._nbinsx, d._nsubframes, d._subframes, mask_path);
  else
    _mask._field = 0;

  strncpy_val(_units, d._units, UnitsSize);
  _units[UnitsSize-1] = 0;
}

DescImage::~DescImage()
{
  if (_mask._ptr) delete _mask._ptr;
}

void DescImage::params(unsigned nbinsx,
                       unsigned nbinsy,
                       int ppxbin,
                       int ppybin)
{
  _nbinsx = nbinsx ? nbinsx : 1;
  _nbinsy = nbinsy ? nbinsy : 1;
  _ppbx = ppxbin;
  _ppby = ppybin;
}

void DescImage::set_units(const char* units)
{
  strncpy_val(_units, units, UnitsSize);
  _units[UnitsSize-1] = 0;
}

void DescImage::set_scale(float scalex, 
                          float scaley)
{
  _unitppx = scalex;
  _unitppy = scaley;
}

void DescImage::add_frame(const SubFrame& f)
{ add_frame(f.x, f.y, f.nx, f.ny, f.r, f.flipx, f.flipy); }

void DescImage::add_frame(unsigned x,
                          unsigned y,
                          unsigned nx,
                          unsigned ny,
                          Rotation r,
                          bool flipx,
                          bool flipy)
{
  if (_nsubframes < MAX_SUBFRAMES) {
    unsigned frame = _nsubframes++;
    _subframes[frame].x     = x;
    _subframes[frame].y     = y;
    _subframes[frame].nx    = nx;
    _subframes[frame].ny    = ny;
    _subframes[frame].r     = r;
    _subframes[frame].flipx = flipx;
    _subframes[frame].flipy = flipy;
  }
  else {
    printf("DescImage::add_frame already at maximum (%d)\n",
           MAX_SUBFRAMES);
  }
}

bool DescImage::xy_bounds(int& x0, int& x1, int& y0, int& y1) const
{
  if ((x0 >= (int)nbinsx()) ||
      (x1 < 0) ||
      (y0 >= (int)nbinsy()) ||
      (y1 < 0))
    return false;
  if (x0 < 0) x0=0;
  if (x1 > (int)nbinsx()) x1=nbinsx();
  if (y0 < 0) y0=0;
  if (y1 > (int)nbinsy()) y1=nbinsy();
  return true;
}

bool DescImage::xy_bounds(int& x0, int& x1, int& y0, int& y1, unsigned fn) const
{
  const SubFrame& f = frame(fn);
  x0 -= f.x;
  x1 -= f.x;
  y0 -= f.y;
  y1 -= f.y;
  if ((x0 >= (int)f.nx) ||
      (x1 < 0) ||
      (y0 >= (int)f.ny) ||
      (y1 < 0))
    return false;
  if (x0 < 0) x0=0;
  if (x1 > (int)f.nx) x1=f.nx;
  if (y0 < 0) y0=0;
  if (y1 > (int)f.ny) y1=f.ny;
  x0 += f.x;
  x1 += f.x;
  y0 += f.y;
  y1 += f.y;
  return true;
}

bool DescImage::rphi_bounds(int& x0, int& x1, int& y0, int& y1,
                            double xc, double yc, double r) const
{
  x0 = xbin(xc - r);
  x1 = xbin(xc + r);
  y0 = ybin(yc - r);
  y1 = ybin(yc + r);
  if ((x0 >= (int)nbinsx()) ||
      (x1 < 0) ||
      (y0 >= (int)nbinsy()) || 
      (y1 < 0))
    return false;
  if (x0 < 0) x0=0;
  if (x1 > (int)nbinsx()) x1=nbinsx();
  if (y0 < 0) y0=0;
  if (y1 > (int)nbinsy()) y1=nbinsy();
  return true;
}

bool DescImage::rphi_bounds(int& x0, int& x1, int& y0, int& y1,
                            double xc, double yc, double r, unsigned fn) const
{
  const SubFrame& f = frame(fn);
  x0 = xbin(xc - r) - f.x;
  x1 = xbin(xc + r) - f.x;
  y0 = ybin(yc - r) - f.y;
  y1 = ybin(yc + r) - f.y;
  if ((x0 >= (int)f.nx) ||
      (x1 < 0) ||
      (y0 >= (int)f.ny) || 
      (y1 < 0))
    return false;
  if (x0 < 0) x0=0;
  if (x1 > (int)f.nx) x1=f.nx;
  if (y0 < 0) y0=0;
  if (y1 > (int)f.ny) y1=f.ny;
  x0 += f.x;
  x1 += f.x;
  y0 += f.y;
  y1 += f.y;
  return true;
}

void DescImage::set_mask(const ImageMask& mask) {
  if (_mask._ptr)
    delete _mask._ptr;

  memset(_mask_path,0,PATHLEN);
  
  _mask._ptr = new ImageMask(mask);
}

void DescImage::deserialize(void* q, const char*& p)
{
  memcpy(q, p, sizeof(DescImage)); 
  p += sizeof(DescImage);
  reinterpret_cast<DescImage*>(q)->_mask._field = 0;
}
