#include "QtChart.hh"
#include "ami/qt/AxisArray.hh"

#include "ami/data/EntryScalar.hh"

#include <QtCore/QTime>

#include "qwt_plot.h"
#include "qwt_scale_draw.h"

namespace Ami {
  namespace Qt {
    class TimeScale : public QwtScaleDraw {
    public:
      TimeScale() {}
    
      virtual QwtText label(double v) const
      {
	time_t t = time_t(v);
	struct tm* tm_ptr = localtime(&t);
	QTime tim(tm_ptr->tm_hour,tm_ptr->tm_min,tm_ptr->tm_sec);
	tim.addMSecs(int(1000*(v-t)));
	return tim.toString();
      }
    };
  };
};

using namespace Ami::Qt;

QtChart::QtChart(const QString&   title,
		 const Ami::EntryScalar& entry,
		 unsigned npoints,
		 const QColor& c) :
  QtBase  (title,entry),
  _cache  (*new EntryScalar(entry.desc())),
  _n      (npoints),
  _current(0),
  _curve  (entry.desc().name())
{
  _curve.setStyle(QwtPlotCurve::Steps);
  _curve.setPen  (QPen(c));
  _curve.setCurveAttribute(QwtPlotCurve::Inverted,true);

  unsigned nb = 2*_n;
  _x = new double[nb];
  _y = new double[nb];

  struct timespec tv;
  clock_gettime(CLOCK_REALTIME,&tv);
  double time = double(tv.tv_sec) + 1.e-9*double(tv.tv_nsec);
  for(unsigned k=0; k<nb; k++) {
    _x[k] = time;
    _y[k] = 0;
  }

  _curve.setRawData(_x,_y,nb+1);  // QwtPlotCurve wants the x-endpoint
  _xinfo = new AxisArray(_x,nb);
}
  
  
QtChart::~QtChart()
{
  _curve.attach(NULL);
  delete _xinfo;
  delete[] _x;
  delete[] _y;
  delete &_cache;
}

void           QtChart::dump  (FILE* f) const
{
  for(unsigned b=_current; b<_current+_n; b++) {
    fprintf(f,"%g %g\n",_x[b],_y[b]);
  }
}

void           QtChart::attach(QwtPlot* p)
{
  _curve.attach(p);
  p->setAxisScaleDraw(QwtPlot::xBottom, new TimeScale);
  p->setAxisLabelRotation (QwtPlot::xBottom, -50.0);
  p->setAxisLabelAlignment(QwtPlot::xBottom, ::Qt::AlignLeft | ::Qt::AlignBottom);
  p->setAxisTitle(QwtPlot::xBottom,"Time [sec]");
}

void           QtChart::update()
{
  const EntryScalar& entry = static_cast<const EntryScalar&>(QtBase::entry());
  double n = entry.entries() - _cache.entries();
  if (n>0) {
    struct timespec tv;
    clock_gettime(CLOCK_REALTIME,&tv);
    double time = double(tv.tv_sec) + 1.e-9*double(tv.tv_nsec);
    _x[_current+0 ] = time;
    _x[_current+_n] = time;
    
    // calculate y
    double y = (n>0) ? (entry.sum() - _cache.sum())/n : 0;
    _cache.setto(entry);
    _y[_current]    = y;
    _y[_current+_n] = y;
    
    if (++_current >= _n)
      _current = 0;

    _curve.setRawData(&_x[_current],&_y[_current],_n);
  }
}

void QtChart::xscale_update()
{
}

void QtChart::yscale_update()
{
}

const AxisArray* QtChart::xinfo() const
{
  return 0;
}
