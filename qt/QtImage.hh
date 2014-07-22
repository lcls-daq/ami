#ifndef AmiQt_QtImage_hh
#define AmiQt_QtImage_hh

#include "ami/qt/QtBase.hh"

#include <QtCore/QVector>
#include <QtGui/QPixmap>

class QwtPlot;
class QColor;
class QSize;
class QGridLayout;

namespace Ami {
  class EntryImage;
  class AbsTransform;
  class DataLock;
  class Semaphore;
  namespace Qt {
    class AxisBins;
    class ImageData;
    class ImageGrid;
    class QtImage : public QtBase {
    public:
      QtImage(const QString&   title,
	      const EntryImage&, 
	      const AbsTransform& x, 
	      const AbsTransform& y,
	      const QColor& c,
              DataLock* lock=0);
      ~QtImage();
    public:
      void        dump  (FILE*) const;
      void        attach(ImageFrame*);
      void        update()        ;
      void        xscale_update() ;
      void        yscale_update() ;
      void        canvas_size(const QSize&,
                              QGridLayout&);
      void        canvas_resize(const QSize&);
      const AxisInfo* xinfo() const;
      const AxisInfo* yinfo() const;
      float           value(unsigned,unsigned) const;
      bool            scalexy() const;
      void            scalexy(bool);
    public:
      QImage*     image(float pedestal,float scale,bool linear=true);
      bool        owns   (QImage*) const;
      void        release(QImage*);
      void        set_color_table(const QVector<QRgb>&);
      void        set_grid_scale(double x,double y);
    private:
      unsigned _x0, _y0;
      unsigned _nx, _ny;
      unsigned _scale;

      enum { NBUFFERS=2 };
      unsigned  _mimage;
      QImage*   _qimage[NBUFFERS];

      AxisBins* _xinfo;
      AxisBins* _yinfo;
      bool       _scalexy;
      ImageGrid* _xgrid;
      ImageGrid* _ygrid;

      DataLock*  _lock;
      Semaphore* _sem;
    };
  };
};

#endif
