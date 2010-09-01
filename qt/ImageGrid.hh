#ifndef AmiQt_ImageGrid_hh
#define AmiQt_ImageGrid_hh

#include <QtGui/QLabel>

namespace Ami {
  namespace Qt {
    class ImageGrid : public QLabel {
    public:
      enum Axis { X, Y };
      enum Origin { TopLeft, Center };
      ImageGrid( Axis, Origin, unsigned sz );
      ~ImageGrid();
    public:
      void resize_grid(unsigned);
      void set_grid_scale(double);
    private:
      void _fill();
    private:
      Axis     _axis;
      Origin   _origin;
      unsigned _size;
      double   _scale;
    };
  };
};

#endif
