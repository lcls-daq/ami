#ifndef AmiQt_QtImage_hh
#define AmiQt_QtImage_hh

#include "ami/qt/QtBase.hh"

#include <QtCore/QVector>
#include <QtGui/QPixmap>

class QwtPlot;
class QColor;

namespace Ami {
  class EntryImage;
  class AbsTransform;
  namespace Qt {
    class AxisBins;
    class ImageData;
    class QtImage : public QtBase {
    public:
      QtImage(const QString&   title,
	      const EntryImage&, 
	      const AbsTransform& x, 
	      const AbsTransform& y,
	      const QColor& c);
      QtImage(const QString&   title,
	      const EntryImage&, 
	      unsigned x0, unsigned y0,
	      unsigned x1, unsigned y1);
      ~QtImage();
    public:
      void        dump  (FILE*) const;
      void        attach(ImageFrame*);
      void        update()        ;
      void        xscale_update() ;
      void        yscale_update() ;
      const AxisInfo* xinfo() const;
      const AxisInfo* yinfo() const;
    public:
      QImage&     image(double scale,bool linear=true);
      void        set_color_table(const QVector<QRgb>&);
    private:
      unsigned _x0, _y0;
      unsigned _nx, _ny;
      unsigned _scale;
      QImage*   _qimage;
      AxisBins* _xinfo;
      AxisBins* _yinfo;
    };
  };
};

#endif
