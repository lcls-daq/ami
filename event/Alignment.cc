#include "ami/event/Alignment.hh"
#include "ami/event/Calib.hh"

#include <sstream>

#include <cmath>
#include <stdio.h>
#include <string.h>

//
//  Read alignment from corner locations and beam position
//  in a Right Handed System.
//  Rotate coordinates to be in x>0, y>0 quadrant
//

using namespace Ami;
using namespace Ami::Alignment;

static const unsigned margin_pixels = 40;

static Rotation as_rotation(double value)
{
  return Rotation(unsigned(value/90.+0.5)%NPHI);
}

static Rotation add_rotations(Rotation r1, Rotation r2)
{
  return Rotation((r1+r2)%NPHI);
}

Element::Element() :
  pname(""),
  pindex(0),
  oname(""),
  oindex(0),
  x0(0.0),
  y0(0.0),
  z0(0.0),
  rot_z(0.0),
  rot_y(0.0),
  rot_x(0.0),
  tilt_z(0.0),
  tilt_y(0.0),
  tilt_x(0.0),
  frame()
{}

Element::Element(const SubFrame& frame) :
  pname(""),
  pindex(0),
  oname(""),
  oindex(0),
  x0(0.0),
  y0(0.0),
  z0(0.0),
  rot_z(0.0),
  rot_y(0.0),
  rot_x(0.0),
  tilt_z(0.0),
  tilt_y(0.0),
  tilt_x(0.0),
  frame(frame)
{}

Element::Element(const std::string& pname,
                 unsigned pindex,
                 const std::string& oname,
                 unsigned oindex) :
  pname(pname),
  pindex(pindex),
  oname(oname),
  oindex(oindex),
  x0(0.0),
  y0(0.0),
  z0(0.0),
  rot_z(0.0),
  rot_y(0.0),
  rot_x(0.0),
  tilt_z(0.0),
  tilt_y(0.0),
  tilt_x(0.0),
  frame()
{}

Element::Element(const std::string& pname,
                 unsigned pindex,
                 const std::string& oname,
                 unsigned oindex,
                 double x0,
                 double y0,
                 double z0,
                 double rot_z,
                 double rot_y,
                 double rot_x,
                 double tilt_z,
                 double tilt_y,
                 double tilt_x,
                 const SubFrame& frame) :
  pname(pname),
  pindex(pindex),
  oname(oname),
  oindex(oindex),
  x0(x0),
  y0(y0),
  z0(z0),
  rot_z(rot_z),
  rot_y(rot_y),
  rot_x(rot_x),
  tilt_z(tilt_z),
  tilt_y(tilt_y),
  tilt_x(tilt_x),
  frame(frame)
{}


Detector::Detector(const Pds::DetInfo& det,
                   const char* dettype, 
                   const char* elemtype,
                   unsigned nelems,
                   double pixel,
                   unsigned pixel_nx,
                   unsigned pixel_ny,
                   unsigned index) :
  _elements(nelems, NULL),
  _margin(margin_pixels * pixel),
  _pixel(pixel),
  _pixel_nx(pixel_nx),
  _pixel_ny(pixel_ny),
  _height(0),
  _width(0),
  _index(index),
  _dettype(dettype),
  _elemtype(elemtype),
  _rotation(D0)
{
  bool offl_type=false;
  FILE* gm = Calib::fopen(det, "geo", "geometry", false, &offl_type);
  if (gm && offl_type==false) {
      printf("Ignoring old-style online geometry constants\n");
      fclose(gm);
      gm = 0;
  }

  if (gm) {
    size_t sz=256;
    char* linep = (char *)malloc(sz);

    bool lerror=false;

    // use for the overall detector frame envelope size
    bool first=true;
    double xmin=0.0;
    double xmax=0.0;
    double ymin=0.0;
    double ymax=0.0;
    double xoffset = 0.0;
    double yoffset = 0.0;
    unsigned count = 0;

    while(true) {
      if (Ami::Calib::get_line(&linep, &sz, gm)<0) break;
      if (strlen(linep)==0) continue;
      if (linep[0]=='#') continue;

      std::string pname;
      unsigned    pindex;
      std::string oname;
      unsigned    oindex;
      double      x0;
      double      y0;
      double      z0;
      double      rot_z;
      double      rot_y;
      double      rot_x;
      double      tilt_z;
      double      tilt_y;
      double      tilt_x;

      std::string sline(linep);
      std::stringstream ss(sline);

      // Note that x <-> y axes are transposed between psana <-> ami
      ss >> pname >> pindex >> oname >> oindex >> y0 >> x0 >> z0
         >> rot_z >> rot_x >> rot_y >> tilt_z >> tilt_x >> tilt_y;

      if ((pname.compare(_dettype) == 0) &&
          (oname.compare(_elemtype) == 0) &&
          (pindex == _index)) {
        if (oindex >= _elements.size()) {
          printf("Unexpected index for %s in geometry: %d\n", oname.c_str(), oindex);
          lerror = true;
          break;
        } else if (_elements[oindex]) {
          printf("Duplicate index for %s in geometry: %d\n", oname.c_str(), oindex);
          lerror = true;
          break;
        } else {
          SubFrame frame = create_subframe(rot_z);
          double x1 = x0 + get_size_x(frame);
          double y1 = y0 + get_size_y(frame);
          if (first) {
            first = false;
            xmin = x0;
            ymin = y0;
            xmax = x1;
            ymax = y1;
          } else {
            if (x0 < xmin)
              xmin = x0;
            if (y0 < ymin)
              ymin = y0;
            if (x1 > xmax)
              xmax = x1;
            if (y1 > ymax)
              ymax = y1;
          }
          _elements[oindex] = new Element(pname, pindex, oname, oindex,
                                          x0, y0, z0,
                                          rot_z, rot_y, rot_x,
                                          tilt_z, tilt_y, tilt_x, frame);
          count++;
        }
      } else if((pname.compare("IP:V1") == 0) &&
                (oname.compare(_dettype) == 0) &&
                (pindex == _index)) {
        _rotation = as_rotation(rot_z);
      }
    }

    // check that we found the expected number of elements in the file
    if (count != _elements.size()) {
      printf("Unexpected number of elements in geometry for %s: %u vs. %zu\n",
             Pds::DetInfo::name(det), count, _elements.size());
      lerror = true;
    }

    if (linep) {
      free(linep);
    }

    if (lerror) {
      printf("No usable geometry found for %s, using default geometry!\n",
             Pds::DetInfo::name(det));
      load_default();
    } else {
      xoffset -= (xmin - _margin);
      yoffset -= (ymin - _margin);
      // Rotate the whole image around the interaction point
      switch (_rotation) {
      case D0:
        _width = calc_size(xmax - xmin);
        _height = calc_size(ymax - ymin);
        for (unsigned i=0; i<_elements.size(); i++) {
          unsigned xbin = calc_binned(_elements[i]->x0 + xoffset);
          unsigned ybin = calc_binned(_elements[i]->y0 + yoffset);
          _elements[i]->frame.x = xbin;
          _elements[i]->frame.y = ybin;
          _elements[i]->frame.r = add_rotations(_elements[i]->frame.r, _rotation);
        }
        break;
      case D90:
        _width = calc_size(ymax - ymin);
        _height = calc_size(xmax - xmin);
        for (unsigned i=0; i<_elements.size(); i++) {
          unsigned xbin = calc_binned(_elements[i]->x0 + xoffset);
          unsigned ybin = calc_binned(_elements[i]->y0 + yoffset);
          _elements[i]->frame.x = ybin;
          _elements[i]->frame.y = _height - xbin - _elements[i]->frame.nx - 1;
          std::swap(_elements[i]->frame.nx, _elements[i]->frame.ny);
          _elements[i]->frame.r = add_rotations(_elements[i]->frame.r, _rotation);
        }
        break;
      case D180:
        _width = calc_size(xmax - xmin);
        _height = calc_size(ymax - ymin);
        for (unsigned i=0; i<_elements.size(); i++) {
          unsigned xbin = calc_binned(_elements[i]->x0 + xoffset);
          unsigned ybin = calc_binned(_elements[i]->y0 + yoffset);
          _elements[i]->frame.x = _width - xbin - _elements[i]->frame.nx - 1;
          _elements[i]->frame.y = _height - ybin - _elements[i]->frame.ny - 1;
          _elements[i]->frame.r = add_rotations(_elements[i]->frame.r, _rotation);
        }
        break;
      case D270:
        _width = calc_size(ymax - ymin);
        _height = calc_size(xmax - xmin);
        for (unsigned i=0; i<_elements.size(); i++) {
          unsigned xbin = calc_binned(_elements[i]->x0 + xoffset);
          unsigned ybin = calc_binned(_elements[i]->y0 + yoffset);
          _elements[i]->frame.x = _width - ybin - _elements[i]->frame.ny - 1;
          _elements[i]->frame.y = xbin;
          std::swap(_elements[i]->frame.nx, _elements[i]->frame.ny);
          _elements[i]->frame.r = add_rotations(_elements[i]->frame.r, _rotation);
        }
        break;
      default:
        break;
      }
    }

    // close the file
    fclose(gm);
  } else {
    printf("No usable geometry found for %s, using default geometry!\n",
           Pds::DetInfo::name(det));
    load_default();
  }
}

Detector::~Detector()
{
  for (unsigned i=0; i<_elements.size(); i++) {
    if (_elements[i]) delete _elements[i];
  }
  _elements.clear();
}

void Detector::add_frames(DescImage& desc, int ppbx, int ppby) const
{
  std::vector<Element*>::const_iterator it;
  for (it=_elements.begin(); it!=_elements.end(); ++it) {
    desc.add_frame(binned_frame((*it)->frame, ppbx, ppby));
  }
}

unsigned Detector::height() const
{
  return _height;
}

unsigned Detector::width() const
{
  return _width;
}

void Detector::load_default()
{
  // margin between elements
  const unsigned margin = 10;
  // cleanup any elements that were created
  for (unsigned i=0; i<_elements.size(); i++) {
    if (_elements[i]) {
      delete _elements[i];
      _elements[i] = NULL;
    }
  }
  // set overall rotation to zero
  _rotation = D0;

  // calculate the grid to stick elements in
  unsigned ngrid = (unsigned) std::sqrt(_elements.size());
  if (ngrid*ngrid < _elements.size()) ngrid++;
  _height = margin;
  _width = ngrid * (_pixel_nx + margin) + margin;

  for (unsigned j=0; j<ngrid; j++) {
    for (unsigned k=0; k<ngrid; k++) {
      unsigned nelem = j*ngrid + k;
      if (nelem >= _elements.size()) {
        break;
      } else if (k == 0) {
        _height += (_pixel_ny + margin);
      }
      SubFrame frame(k * (_pixel_nx + margin) + margin,
                     j * (_pixel_ny + margin) + margin,
                     _pixel_nx, _pixel_ny, D0);
      _elements[nelem] = new Element(frame);
    }
  }
}

unsigned Detector::calc_size(double value) const
{
  return calc_binned(value + 2 * _margin);
}

unsigned Detector::calc_binned(double value) const
{
  return (unsigned) ((value / _pixel) + 0.5);
}

double Detector::get_size_x(const SubFrame& frame) const
{
  switch(frame.r) {
  case D0:
  case D180:
    return _pixel * _pixel_nx;
  case D90:
  case D270:
    return _pixel * _pixel_ny;
  default:
    return 0.0;
  }
}

double Detector::get_size_y(const SubFrame& frame) const
{
  switch(frame.r) {
  case D0:
  case D180:
    return _pixel * _pixel_ny;
  case D90:
  case D270:
    return _pixel * _pixel_nx;
  default:
    return 0.0;
  }
}

SubFrame Detector::create_subframe(double rotation) const
{
  Rotation rot = as_rotation(rotation);
  switch(rot) {
  case D0:
  case D180:
    return SubFrame(0, 0, _pixel_nx, _pixel_ny, rot);
  case D90:
  case D270:
    return SubFrame(0, 0, _pixel_ny, _pixel_nx, rot);
  default:
    return SubFrame();
  }
}

SubFrame Detector::binned_frame(const SubFrame& frame, int ppbx, int ppby) const
{
  return SubFrame(frame.x / ppbx, frame.y / ppby,
                  (frame.nx + ppbx - 1) / ppbx, (frame.ny + ppby - 1) / ppby,
                  frame.r);
}

Rotation Detector::rotation() const
{
  return _rotation;
}
