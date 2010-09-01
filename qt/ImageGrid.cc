#include "ami/qt/ImageGrid.hh"

#include <QtGui/QImage>
#include <QtGui/QPainter>

#include <math.h>

using namespace Ami::Qt;

static const unsigned grid_width = 35;

ImageGrid::ImageGrid( Axis a, Origin o, unsigned sz ) : 
  _axis  (a),
  _origin(o),
  _size  (sz),
  _scale (1)
{
  setAlignment(::Qt::AlignLeft | ::Qt::AlignTop);
  _fill();
}

ImageGrid::~ImageGrid()
{
}

void ImageGrid::_fill()
{
  QImage* image = new QImage(grid_width, _size, QImage::Format_ARGB32);
  
  const double major_width = 2;

  double major_step, fstep, y0;
  if (_origin == Center) {
    double len = 0.5*double(_size)*_scale;
    major_step = pow(10.,floor(log10(len))-1.);
    const unsigned max_steps = 6;
    fstep = len/(major_step*double(max_steps));
    y0 = -len;
  }
  else {
    double len = double(_size)*_scale;
    major_step = pow(10.,floor(log10(len))-1.);
    const unsigned max_steps = 13;
    fstep = len/(major_step*double(max_steps));
    y0 = 0;
  }

  //  we have step size to within a factor of 10
  if      (fstep > 5)
    major_step *= 10;
  else if (fstep > 2)
    major_step *= 5;
  else
    major_step *= 2;

  QRgb bg = QPalette().color(QPalette::Window).rgb();
  image->fill(bg);

  QRgb fg = qRgb(0,0,0);

  QRgb* b = reinterpret_cast<QRgb*>(image->bits());
  double y = y0;
  for(unsigned i=0; i<_size; i++, b+=grid_width) {
    b[0] = fg;
    b[1] = fg;

    QRgb v;
    double prox = major_width - fabs(drem(y,major_step))/_scale;
    if (prox < 0) v = bg;
    else if (prox < 1) {
      int g = int(double(qGray(bg))*(1-prox) + double(qGray(fg))*prox);
      v = qRgb(g,g,g);
    }
    else v = fg;

    b[2] = v;
    b[3] = v;
    b[4] = v;
    y += _scale;
  }
  
  //  setMinimumSize(image->size());
  QTransform transform(QTransform().rotate(_axis==X ? 180 : 0, ::Qt::XAxis));
  QPixmap pixmap(QPixmap::fromImage(*image).transformed(transform));
  QPainter painter(&pixmap);
  painter.setLayoutDirection(::Qt::LeftToRight);
  QFont f(painter.font());
  f.setPointSize(8);
  painter.setFont(f);

  int nsteps;
  double g0, v0;
  if (_origin==Center) {
    int n  = int(0.5*double(_size)*_scale/major_step);
    nsteps = 2*n + 1;
    v0     = double(n)*major_step;
    g0     = 0.5*double(_size)*_scale - v0;
  }
  else {
    int n  = int(double(_size)*_scale/major_step);
    nsteps = n + 1;
    if (_axis == X) {
      v0     = major_step*double(n);
      g0     = double(_size)*_scale - double(n)*major_step;
    }
    else {
      v0     = 0;
      g0     = 0;
    }
  }
  //  int nsteps = int(len/major_step);
  //  for(int i=-nsteps; i<=nsteps; i++) {
  for(int i=0; i<nsteps; i++) {
//     QRectF r(8, (double(i)*major_step+len)/_scale-5,grid_width-8,10);
//     double v = double(i)*major_step;
//     if (_axis==X) v = -v;
//     painter.drawText(r, ::Qt::AlignLeft|::Qt::AlignVCenter, QString::number(v));
    QRectF r(8, (g0+double(i)*major_step)/_scale-5,grid_width-8,10);
    double v = double(i)*major_step;
    if (_axis==X) v = v0-v;
    else          v = v-v0;
    painter.drawText(r, ::Qt::AlignLeft|::Qt::AlignVCenter, QString::number(v));
  }

  if (_axis==X)
    setPixmap(pixmap.transformed(QTransform().rotate(90)));
  else
    setPixmap(pixmap);

  delete image;
}

void ImageGrid::resize_grid(unsigned sz)
{
  _size = sz;
  _fill();
}

void ImageGrid::set_grid_scale(double s)
{
  _scale = s;
  _fill();
}
