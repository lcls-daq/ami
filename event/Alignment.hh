#ifndef Ami_Alignment_hh
#define Ami_Alignment_hh

#include "ami/data/DescImage.hh"
#include <string>
#include <vector>

namespace Pds { class DetInfo; };

/* 
 *  Class to convert optical alignment measurements for display
 *  Optical alignment coordinates assume device in module 1 position
 */

namespace Ami {
  namespace Alignment {
    class Element {
    public:
      Element();
      Element(const std::string& pname,
              unsigned pindex,
              const std::string& oname,
              unsigned oindex);
      Element(const std::string& pname,
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
              const SubFrame& frame);

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
      SubFrame    frame;
    };

    class Detector {
    public:
      Detector(const Pds::DetInfo& det,
               const char* dettype,
               const char* elemtype,
               unsigned nelems,
               double pixel,
               unsigned pixel_nx,
               unsigned pixel_ny,
               unsigned index=0);
      virtual ~Detector();
      Rotation rotation() const;
      unsigned height() const;
      unsigned width() const;
      void add_frames(DescImage& desc, int ppbx=1, int ppby=1) const;
    protected:
      unsigned calc_size(double value) const;
      unsigned calc_binned(double value) const;
      double get_size_x(const SubFrame& frame) const;
      double get_size_y(const SubFrame& frame) const;
      SubFrame create_subframe(double rotation) const;
      SubFrame binned_frame(const SubFrame& frame, int ppbx, int ppby) const;
    protected:
      std::vector<Element*> _elements;
      double                _margin;
      double                _pixel;
      unsigned              _pixel_nx;
      unsigned              _pixel_ny;
      unsigned              _height;
      unsigned              _width;
    private:
      unsigned              _index;
      std::string           _dettype;
      std::string           _elemtype;
      double                _rotation;
    };
  }
}

#endif
