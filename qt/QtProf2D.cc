#include "QtProf2D.hh"
#include "ami/qt/AxisArray.hh"
#include "ami/qt/ImageColorControl.hh"
#include "ami/qt/Defaults.hh"

#include "ami/data/AbsTransform.hh"
#include "ami/data/EntryProf2D.hh"

#include "qwt_plot.h"
#include "qwt_plot_layout.h"
#include "qwt_raster_data.h"
#include "qwt_scale_widget.h"
#include "qwt_color_map.h"

#include <QtGui/QPen>
namespace Ami {
  namespace Qt {
    class QtProf2D::DataCache : public QwtRasterData {
    public:
      DataCache(const QtBase& base) : _base(base) 
      {
        const DescProf2D& desc = static_cast<const EntryProf2D&>(base.entry()).desc();
        _idx = double(desc.nxbins())/(desc.xup()-desc.xlow());
        _idy = double(desc.nybins())/(desc.yup()-desc.ylow());
        setBoundingRect(QwtDoubleRect(desc.xlow(),desc.ylow(),
                                      desc.xup()-desc.xlow(),
                                      desc.yup()-desc.ylow()));
        update();
      }
      ~DataCache() {}
    public:
      //  very slow
      double value(double x,double y) const {
        const EntryProf2D& e = static_cast<const EntryProf2D&>(_base.entry());
        //  QwtPlot probes the upper left corner of each bin
        int ix = int((x-e.desc().xlow())*_idx+0.5);
        int iy = int((y-e.desc().ylow())*_idy-0.5);
        if (ix>=0 && ix<int(e.desc().nxbins()) &&
            iy>=0 && iy<int(e.desc().nybins()))
          return e.zmean(ix,iy);
        else
          return 0;
      }
      QwtRasterData* copy() const {
        return const_cast<DataCache*>(this);
      }
      QwtDoubleInterval range() const {
        return QwtDoubleInterval(_vlo,_vhi);
      }
      QSize rasterHint ( const QwtDoubleRect& rect) const {
        const DescProf2D& desc = static_cast<const EntryProf2D&>(_base.entry()).desc();
        return QSize(desc.nxbins(),desc.nybins());
      }
    public:
      void update() {
        const EntryProf2D& e = static_cast<const EntryProf2D&>(_base.entry());
        const DescProf2D&  d = e.desc();
        bool linit=true;
        _vlo = 0; _vhi = 1;
        for(unsigned i=0; i<d.nybins(); i++)
          for(unsigned j=0; j<d.nxbins(); j++) {
            double n = e.nentries(j,i);
            if (n) {
              double z = e.zsum(j,i)/n;
              if (linit) {
                _vlo = _vhi = z;
                linit = false;
              }
              else {
                if (z < _vlo) _vlo = z;
                if (z > _vhi) _vhi = z;
              }
            }
          }
      }
    private:
      double _idx, _idy;
      double _vlo, _vhi;
      const QtBase& _base;
    };
  };
};

using namespace Ami::Qt;

QtProf2D::QtProf2D(const QString&   title,
	       const Ami::EntryProf2D& entry,
               const AbsTransform&,
               const AbsTransform&,
               const QColor&) :
  QtBase(title,entry),
  _curve(entry.desc().name()),
  _z    (new DataCache(*this))
{
  _curve.setData    (*_z);

  _curve.setDisplayMode(QwtPlotSpectrogram::ImageMode, true);
  _curve.setDefaultContourPen(QPen(::Qt::NoPen));

#if 0
  const QVector<QRgb>& palette = ImageColorControl::current_color_table();
  QwtLinearColorMap map = QwtLinearColorMap(QColor(palette[0]),QColor(palette[palette.size()-1]));
  for(int k=0; k<palette.size(); k++)
    map.addColorStop( double(k)/double(palette.size()-1), QColor(palette[k]));
#else
  QwtLinearColorMap map = QwtLinearColorMap(::Qt::black, ::Qt::white); 
  for(unsigned k=0; k<40; k++)
    map.addColorStop(double(  0+k)/255., QColor(k*6,0,0));
  for(unsigned k=0; k<86; k++)
    map.addColorStop(double( 43+k)/255., QColor(255-k*3,k*3,0));
  for(unsigned k=0; k<86; k++)
    map.addColorStop(double(129+k)/255., QColor(0,255-k*3,k*3));
  for(unsigned k=0; k<40; k++)
    map.addColorStop(double(215+k)/255., QColor(k*3,0,255-k*3));
  map.addColorStop(1.,QColor(255,255,255));
#endif
  _curve.setColorMap(map);

  connect(this, SIGNAL(color_changed()), this, SLOT(change_color()));
}
  
  
QtProf2D::~QtProf2D()
{
  _curve.attach(NULL);
}

void           QtProf2D::dump  (FILE* f) const
{
  int prec = Defaults::instance()->save_precision();
  const DescProf2D&  _desc  = static_cast<const DescProf2D& >(entry().desc());
  const EntryProf2D& _e     = static_cast<const EntryProf2D&>(entry());
  fprintf(f,"%.*g %.*g %.*g %.*g\n",
          prec,_desc.xlow(),
          prec,_desc.xup(),
          prec,_desc.ylow(),
          prec,_desc.yup());
  for(unsigned iy=0; iy<_desc.nybins(); iy++) {
    for(unsigned ix=0; ix<_desc.nxbins(); ix++)
      fprintf(f,"%.*g ",prec,_e.zmean(ix,iy));
    fprintf(f,"\n");
  }
}

void           QtProf2D::attach(QwtPlot* p)
{
  _curve.attach(p);
  if (p) {
    const DescProf2D& _desc = static_cast<const EntryProf2D&>(entry()).desc();
    p->setAxisTitle(QwtPlot::xBottom, _desc.xtitle());
    p->setAxisScale(QwtPlot::xBottom, _desc.xlow(), _desc.xup());
    p->setAxisTitle(QwtPlot::yLeft  , _desc.ytitle());
    p->setAxisScale(QwtPlot::yLeft  , _desc.ylow(), _desc.yup());

    QwtScaleWidget* a = p->axisWidget(QwtPlot::yRight);
    a->setColorBarEnabled(true);
    a->setColorMap(_z->range(),_curve.colorMap());
    p->setAxisScale(QwtPlot::yRight, _z->range().minValue(), _z->range().maxValue());
    p->enableAxis  (QwtPlot::yRight);
    p->plotLayout()->setAlignCanvasToScales(true);  

    _colorBar = a;
    _plot = p;
  }
}

void           QtProf2D::update()
{
  if (!entry().valid()) {
#ifdef DBUG
    printf("QtProf2D::update invalid\n");
#endif
    return;
  }

  emit color_changed();
}

void           QtProf2D::change_color()
{
  _z->update();
  _colorBar->setColorMap(_z->range(),_curve.colorMap());
  _plot->setAxisScale(QwtPlot::yRight, _z->range().minValue(), _z->range().maxValue());
}

void QtProf2D::xscale_update()
{
}

void QtProf2D::yscale_update()
{
}

const AxisInfo* QtProf2D::xinfo() const
{
  return 0;
}

double QtProf2D::normalization() const
{
  return static_cast<const EntryProf2D&>(entry()).info(EntryProf2D::Normalization);
}
