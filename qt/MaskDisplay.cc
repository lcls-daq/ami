#include "MaskDisplay.hh"

#include "ami/qt/ImageColorControl.hh"
#include "ami/qt/ImageXYControl.hh"
#include "ami/qt/Cursors.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/MaskFrame.hh"
#include "ami/qt/QtImage.hh"
#include "ami/qt/ImageMarker.hh"

#include "ami/data/AbsTransform.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EntryImage.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QActionGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QFileDialog>
#include <QtGui/QInputDialog>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>
#include <QtGui/QToolBar>
#include <QtGui/QPolygon>
#include <QtGui/QPainter>

#include <sys/uio.h>
#include <time.h>
#include <fstream>

static void interpolate(double& x, double& y,
                        double m,double b,int ncols,int nrows)
{
  if (x<0) {
    double dx = x;
    double dy = m*dx;
    x += dx;
    y += dy;
  }
  if (x>double(ncols-1)) {
    double dx = double(ncols)-1-x;
    double dy = m*dx;
    x += dx;
    y += dy;
  }

  if (m!=0) {
    if (y<0) {
      double dy = y;
      double dx = dy/m;
      x += dx;
      y += dy;
    }
    if (y>double(nrows-1)) {
      double dy = double(nrows-1)-y;
      double dx = dy/m;
      x += dx;
      y += dy;
    }
  }
}

namespace Ami {
  namespace Qt {
    class RectHandle : public Cursors,
                       public ImageMarker {
    public:
      RectHandle (MaskDisplay& p) : _parent(p), _active(false) {}
      ~RectHandle() {}
    public:
      void mousePressEvent  (double dx, double dy)
      {
        int x((int)dx), y((int)dy);
        if (x < 0) x = 0;
        else if (x > _xmax) x = _xmax;
        if (y < 0) y = 0;
        else if (y > _ymax) y = _ymax;
       _x0=x; _y0=y;
      }
      void mouseMoveEvent   (double dx, double dy)
      {
        int x((int)dx), y((int)dy);
        if (x < 0) x = 0;
        else if (x > _xmax) x = _xmax;
        if (y < 0) y = 0;
        else if (y > _ymax) y = _ymax;
        _x1=x; _y1=y;
        _active = true;
      }
      void mouseReleaseEvent(double x, double y)
      {
        mouseMoveEvent(x,y);
        _active = false;
        _parent.rect(_x0,_y0,_x1,_y1);
        _parent.push();
      }
    public:
      void draw(QImage& image)
      {
        const QSize& sz = image.size();
        _xmax = sz.width ();
        _ymax = sz.height();

        if (_active) {
          const unsigned char c = 0xff;
          
          int jlo,jhi;
          if (_x0<_x1)  { jlo=_x0; jhi=_x1; }
          else          { jlo=_x1; jhi=_x0; }

          int klo,khi;
          if (_y0<_y1)  { klo=_y0; khi=_y1; }
          else          { klo=_y1; khi=_y0; }

          { unsigned char* cc0 = image.scanLine(klo) + jlo;
            unsigned char* cc1 = image.scanLine(khi) + jlo;
            for(int j=jlo; j<=jhi; j++) {
              *cc0++ = c;
              *cc1++ = c;
            }
          }

          { for(int k=klo; k<khi; k++) {
              *(image.scanLine(k+1)+jlo) = c;
              *(image.scanLine(k+1)+jhi) = c;
            }
          }
        }
      }
    private:
      MaskDisplay& _parent;
      int _x0;
      int _y0;
      int _x1;
      int _y1;
      int _xmax, _ymax;
      bool _active;
    };
    class RingHandle : public Cursors,
                       public ImageMarker {
    public:
      RingHandle (MaskDisplay& p) : _parent(p), _active(false) {}
      ~RingHandle() {}
    public:
      void mousePressEvent  (double dx, double dy)
      {
        int x((int)dx), y((int)dy);
        if (x < 0) x = 0;
        else if (x > _xmax) x = _xmax;
        if (y < 0) y = 0;
        else if (y > _ymax) y = _ymax;
       _x0=x; _y0=y;
      }
      void mouseMoveEvent   (double dx, double dy)
      {
        int x((int)dx), y((int)dy);
        /*
        if (x < 0) x = 0;
        else if (x > _xmax) x = _xmax;
        if (y < 0) y = 0;
        else if (y > _ymax) y = _ymax;
        */
        _x1=x; _y1=y;
        _active = true;
      }
      void mouseReleaseEvent(double x, double y)
      {
        mouseMoveEvent(x,y);
        _active = false;
        _parent.ring(_x0,_y0,_x1,_y1);
        _parent.push();
      }
    public:
      void draw(QImage& image)
      {
        const QSize& sz = image.size();
        _xmax = sz.width ();
        _ymax = sz.height();

        if (_active) {
          const unsigned char c = 0xff;
          
          double rsq = pow(double(_x1-_x0),2)+pow(double(_y1-_y0),2);
          double r   = sqrt(rsq);
          int ir = int(r);

          int klo=_y0-ir,khi=_y0+ir;
          if (klo<0) klo=0;
          if (khi>=_ymax) khi=_ymax-1;

          for(int k=klo; k<=khi; k++) {
            int dx = int(sqrt(rsq - pow(double(k-_y0),2)));
            int jlo=_x0-dx,jhi=_x0+dx;
            if (jlo>=0)    *(image.scanLine(k)+jlo)=c;
            if (jhi<_xmax) *(image.scanLine(k)+jhi)=c;
          }
        }
      }
    private:
      MaskDisplay& _parent;
      int _x0;
      int _y0;
      int _x1;
      int _y1;
      int _xmax, _ymax;
      bool _active;
    };
    class PolyHandle : public Cursors,
                       public ImageMarker {
    public:
      PolyHandle (MaskDisplay& p) : _parent(p), _active(false) {}
      ~PolyHandle() {}
    public:
      void mousePressRight  (double dx, double dy)
      {
        if (_active) {
          int x((int)dx), y((int)dy);
          _poly << QPoint(x,y);
          if (_poly.size()>2) {
            _parent.poly(_poly);
            _parent.push();
          }
          _active=false;
        }
      }
      void mousePressEvent  (double dx, double dy)
      {
        int x((int)dx), y((int)dy);
        if (!_active) {
          if (x < 0) x = 0;
          else if (x > _xmax) x = _xmax;
          if (y < 0) y = 0;
          else if (y > _ymax) y = _ymax;
          _poly = QPolygon();
          _poly << QPoint(x,y);
          _active=true;
        }
        else {
          _poly << QPoint(x,y);
        }
      }
      void mouseMoveEvent   (double dx, double dy)
      {
      }
      void mouseReleaseEvent(double x, double y)
      {
      }
    public:
      void draw(QImage& image)
      {
        const QSize& sz = image.size();
        _xmax = sz.width ();
        _ymax = sz.height();
        if (_active) {
          if (_poly.size()>0) {
            QImage output = image.convertToFormat(::QImage::Format_RGB32);
            QPainter painter(&output);
            painter.setPen(QColor(0xff,0xff,0xff));
            painter.drawPolyline(_poly);
            image = output.convertToFormat(::QImage::Format_Indexed8,
                                           image.colorTable());
          }
        }
      }
    private:
      MaskDisplay& _parent;
      bool         _active;
      int _x0;
      int _y0;
      int _xmax, _ymax;
      QPolygon _poly;
    };
    class PixlHandle : public Cursors,
                       public ImageMarker {
    public:
      PixlHandle (MaskDisplay& p) : _parent(p) {}
      ~PixlHandle() {}
    public:
      void mousePressEvent  (double dx, double dy)
      {
        int x((int)dx), y((int)dy);
        if (x < 0) x = 0;
        else if (x > _xmax) x = _xmax;
        if (y < 0) y = 0;
        else if (y > _ymax) y = _ymax;
       _x0=x; _y0=y;
       _parent.pixl(_x0,_y0,_x0,_y0);
      }
      void mouseMoveEvent   (double dx, double dy)
      {
        int x((int)dx), y((int)dy);
        _parent.pixl(_x0,_y0,x,y);
        _x0=x; _y0=y;
      }
      void mouseReleaseEvent(double x, double y)
      {
        mouseMoveEvent(x,y);
        _parent.push();
      }
    public:
      void draw(QImage& image)
      {
        const QSize& sz = image.size();
        _xmax = sz.width ();
        _ymax = sz.height();
      }
    private:
      MaskDisplay& _parent;
      int _x0;
      int _y0;
      int _x1;
      int _y1;
      int _xmax, _ymax;
    };
  };
};

using namespace Ami::Qt;

static const double no_scale[] = {0, 1000};

MaskDisplay::MaskDisplay(bool grab) :
  QtPWidget(0),
  _bkg     (0),
  _mask    (0),
  _entry   (0),
  _empty   (0),
  _queue_pos(0)
{
  _menu_bar = new QMenuBar(this);
  _plotBox = new QGroupBox("Plot");
  _xyrange = new ImageXYControl(this,"XY");
  _zrange  = new ImageColorControl(this,"Z");
  _plot    = new MaskFrame(this,*_xyrange,*_zrange);

  _rect_handle = new RectHandle(*this);
  _ring_handle = new RingHandle(*this);
  _poly_handle = new PolyHandle(*this);
  _pixl_handle = new PixlHandle(*this);

  _layout();
}

void MaskDisplay::_layout()
{
  {
    QMenu* file_menu = new QMenu("File");
    file_menu->addAction("Load mask"      , this, SLOT(load_mask()));
    file_menu->addAction("Save mask"      , this, SLOT(save_mask()));
    file_menu->addAction("Load backg"     , this, SLOT(load_bkg ()));
    file_menu->addSeparator();
    _menu_bar->addMenu(file_menu);
  }
  {
    QMenu* edit_menu = new QMenu("Edit");
    edit_menu->addAction("Clear"          , this, SLOT(clear_mask()));
    edit_menu->addAction("Fill"           , this, SLOT(fill_mask()));
    edit_menu->addAction("Invert"         , this, SLOT(invert_mask()));
    _menu_bar->addMenu(edit_menu);
  }
  {
    _bkg_action  = _menu_bar->addAction("Hide bkg", this, SLOT(toggle_bkg()));
    _bkg_is_visible = true;
    _undo_action = _menu_bar->addAction("Undo"    , this, SLOT(undo()));
    _redo_action = _menu_bar->addAction("Redo"    , this, SLOT(redo()));
  }

  QToolBar* tools = new QToolBar(0);
  _comb_action = tools->addAction("Exclude", this, SLOT(toggle_comb()));
  _comb_is_plus = false;
//   (_rect_action = tools->addAction("XY"              , this, SLOT(toggle_rect())))->setCheckable(true);
//   (_ring_action = tools->addAction(QString("%1%2")
//                                    .arg(QChar(0x3c1))
//                                    .arg(QChar(0x3c6)), this, SLOT(toggle_ring())))->setCheckable(true);
  (_rect_action = tools->addAction(QChar(0x25a1)     , this, SLOT(toggle_rect())))->setCheckable(true);
  (_ring_action = tools->addAction(QChar(0x25cb)     , this, SLOT(toggle_ring())))->setCheckable(true);
  (_poly_action = tools->addAction(QChar(0x2608)     , this, SLOT(toggle_poly())))->setCheckable(true);
  (_pixl_action = tools->addAction(QChar(0x271b)     , this, SLOT(toggle_pixl())))->setCheckable(true);
  _active_action = NoAction;
    

  QVBoxLayout* mainLayout = new QVBoxLayout;
  mainLayout->setSpacing(1);
  {
    QVBoxLayout* layout1 = new QVBoxLayout;
    { QHBoxLayout* layout2 = new QHBoxLayout;
      layout2->addWidget(_menu_bar);
      layout2->addStretch();
      layout2->addWidget(tools);
      layout1->addLayout(layout2); }
    layout1->addWidget(_plot);
    _plotBox->setLayout(layout1);
    mainLayout->addWidget(_plotBox,1); }
  { QHBoxLayout* layout2 = new QHBoxLayout;
    layout2->addWidget(_xyrange);
    layout2->addStretch();
    layout2->addWidget(_zrange);
    mainLayout->addLayout(layout2); }
  setLayout(mainLayout);

  connect(this   , SIGNAL(redraw()) , _plot      , SLOT(replot()));
  connect(_xyrange, SIGNAL(windowChanged()), _plot, SLOT(replot()));

  _undo_action->setEnabled(false);
  _redo_action->setEnabled(false);
}

MaskDisplay::~MaskDisplay()
{
}

void MaskDisplay::setup(QtBase* base, const QString& fname)
{
  if (_mask) 
    delete _mask;

  const EntryImage& entry = static_cast<const EntryImage&>(base->entry());
  const DescImage&      d = entry.desc(); 
  _mask = new ImageMask(d.nbinsy(),d.nbinsx(),
                        d.nframes(),&d.frame(0),
                        qPrintable(fname));
  _fname = fname;

  _queue.clear();
  _queue_pos = 0;
  push();

  if (_entry)
    delete _entry;
  _entry = new EntryImage(d);
  _entry->setto(entry);

  if (_empty)
    delete _empty;
  _empty = new EntryImage(d);

  if (_bkg)
    delete _bkg;

  _bkg = new QtImage(d.name(),
                     _bkg_is_visible ? *_entry : *_empty,
                     Ami::AbsTransform::null(),
                     Ami::AbsTransform::null(),
                     QColor(0,0,0));


  _plot->attach_mask(*_mask);
  _plot->attach_bkg (*_bkg);

  emit redraw();
}

void MaskDisplay::clear_mask()
{
  _mask->clear();
  push();
  emit redraw();
}

void MaskDisplay::fill_mask()
{
  _mask->fill();
  push();
  emit redraw();
}

void MaskDisplay::invert_mask()
{
  _mask->invert();
  push();
  emit redraw();
}

void MaskDisplay::load_mask()
{
  // incomplete
  push();
  emit redraw();
}

void MaskDisplay::save_mask()
{
  QString def = _fname;
  if (def.isEmpty())
    def = QString("%1.msk").arg(_entry->desc().name());
  QString fname =
    QFileDialog::getSaveFileName(this,"Save File As (.msk)",
                                 def,".msk");
  if (!fname.isNull()) {
    _fname = fname;
    FILE* f = fopen(qPrintable(fname),"w");
    if (!f)
      QMessageBox::warning(this, "Save data",
                           QString("Error opening %1 for writing").arg(fname));
    else {
      for(unsigned j=0; j<_mask->rows(); j++) {
        for(unsigned i=0; i<_mask->cols(); i++)
          fprintf(f,"%f ",_mask->rowcol(j,i) ? 1.:0.);
        fprintf(f,"\n");
      }        
      fclose(f);
    }
  }
}

void MaskDisplay::load_bkg ()
{
  // incomplete
}

void MaskDisplay::toggle_bkg ()
{
  if (_bkg) {
    if (_bkg_is_visible) {
      _bkg->entry(*_empty);
      _bkg_action->setText("Show bkg");
      _bkg_is_visible = false;
    }
    else {
      _bkg->entry(*_entry);
      _bkg_action->setText("Hide bkg");
      _bkg_is_visible = true;
    }
    emit redraw();
  }
}

void MaskDisplay::toggle_comb ()
{
  if (_comb_is_plus) {
    _comb_action->setText("Exclude");
    _comb_is_plus = false;
  }
  else {
    _comb_action->setText("Include");
    _comb_is_plus = true;
  }
}

void MaskDisplay::toggle_rect()
{
  _active_action = RectAction; 
  _rect_action->setChecked(true );
  _ring_action->setChecked(false);
  _poly_action->setChecked(false);
  _pixl_action->setChecked(false);
  _plot->set_cursor_input (_rect_handle);
  _plot->attach_marker(*_rect_handle);
  emit redraw();
}

void MaskDisplay::toggle_ring() 
{
  _active_action = RingAction; 
  _rect_action->setChecked(false);
  _ring_action->setChecked(true );
  _poly_action->setChecked(false);
  _pixl_action->setChecked(false);
  _plot->set_cursor_input (_ring_handle);
  _plot->attach_marker(*_ring_handle);
  emit redraw();
}

void MaskDisplay::toggle_poly()
{
  _active_action = PolyAction; 
  _rect_action->setChecked(false);
  _ring_action->setChecked(false);
  _poly_action->setChecked(true );
  _pixl_action->setChecked(false);
  _plot->set_cursor_input (_poly_handle);
  _plot->attach_marker(*_poly_handle);
  emit redraw();
}

void MaskDisplay::toggle_pixl()
{
  _active_action = PixlAction; 
  _rect_action->setChecked(false);
  _ring_action->setChecked(false);
  _poly_action->setChecked(false);
  _pixl_action->setChecked(true );
  _plot->set_cursor_input (_pixl_handle);
  _plot->attach_marker(*_pixl_handle);
  emit redraw();
}

void MaskDisplay::rect(int x0, int y0, int x1, int y1)
{
  ImageMask m(_mask->rows(),_mask->cols());
  
  m.clear();
  for(int j=y0; j<y1; j++)
    for(int i=x0; i<x1; i++)
      m.fill(j,i);

  if (_comb_is_plus)
    *_mask |= m;
  else
    *_mask &= ~m;

  emit redraw();
}

void MaskDisplay::ring(int x0, int y0, int x1, int y1)
{
  ImageMask m(_mask->rows(),_mask->cols());
  
  double rsq = pow(double(x1-x0),2)+pow(double(y1-y0),2);
  double r   = sqrt(rsq);
  int ir = int(r);

  int klo=y0-ir,khi=y0+ir;
  if (klo<0) klo=0;
  if (khi>=int(m.rows())) khi=m.rows()-1;

  m.clear();
  
  for(int k=klo; k<=khi; k++) {
    int dx = int(sqrt(rsq - pow(double(k-y0),2)));
    int jlo=x0-dx,jhi=x0+dx;
    if (jlo<0) jlo=0;
    if (jhi>=int(m.cols())) jhi=m.cols()-1;
    
    for(int j=jlo; j<jhi; j++)
      m.fill(k,j);
  }

  if (_comb_is_plus) 
    *_mask |= m;
  else
    *_mask &= ~m;

  emit redraw();
}

void MaskDisplay::poly(const QPolygon& poly)
{
  QImage image(_mask->cols(),_mask->rows(),::QImage::Format_RGB32);
  image.fill(0);
  QPainter painter(&image);
  painter.setPen(QColor(0xff,0xff,0xff));
  painter.setBrush(QColor(0xff,0xff,0xff));
  painter.drawPolygon(poly);

  ImageMask m(_mask->rows(),_mask->cols());
  for(unsigned k=0; k<_mask->rows(); k++) {
    for(unsigned j=0; j<_mask->cols(); j++) {
      if ((image.pixel(j,k)&0xffffff)==0)
        m.clear(k,j);
      else
        m.fill(k,j);
    }
  }

  if (_comb_is_plus)
    *_mask |= m;
  else
    *_mask &= ~m;

  emit redraw();
}

void MaskDisplay::pixl(int x0, int y0, int x1, int y1)
{
  ImageMask m(_mask->rows(),_mask->cols());
  
  double s = double(y1-y0)/double(x1-x0);
  double b = double(y0)-s*double(x0);

  double xx0 = double(x0);
  double xx1 = double(x1);
  double yy0 = double(y0);
  double yy1 = double(y1);
  interpolate(xx0,yy0,s,b,m.cols(),m.rows());
  interpolate(xx1,yy1,s,b,m.cols(),m.rows());

  m.clear();
    
  if (fabs(s)<1) {
    if (xx0<xx1)
      for(int i=int(xx0+0.5); i<=int(xx1+0.5); i++, yy0+=s)
        m.fill(int(yy0+0.5),i);
    else
      for(int i=int(xx1+0.5); i<=int(xx0+0.5); i++, yy1+=s)
        m.fill(int(yy1+0.5),i);
  }
  else {
    s = 1/s;
    if (yy0<yy1)
      for(int i=int(yy0+0.5); i<=int(yy1+0.5); i++, xx0+=s)
        m.fill(i,int(xx0+0.5));
    else
      for(int i=int(yy1+0.5); i<=int(yy0+0.5); i++, xx1+=s)
        m.fill(i,int(xx1+0.5));
  }

  if (_comb_is_plus)
    *_mask |= m;
  else
    *_mask &= ~m;

  emit redraw();
}

void MaskDisplay::push()
{
  if (_queue_pos+1 < _queue.size())
    _queue.erase(_queue.begin()+_queue_pos+1,_queue.end());

  _queue.push_back(*_mask);
  _queue_pos = _queue.size()-1;

  if (_queue.size()>MaxQueue) {
    unsigned n = _queue.size()-MaxQueue;
    _queue_pos -= n;
    _queue.erase(_queue.begin(),_queue.begin()+n);
  }

  _redo_action->setEnabled(false);
  if (_queue_pos)
    _undo_action->setEnabled(true);
}

void MaskDisplay::undo()
{
  if (_queue_pos) {
    *_mask = _queue[--_queue_pos];
    _redo_action->setEnabled(true);
    if (_queue_pos==0)
      _undo_action->setEnabled(false);
    emit redraw();
  }
}

void MaskDisplay::redo()
{
  if (_queue_pos+1 < _queue.size()) {
    *_mask = _queue[++_queue_pos];
    _undo_action->setEnabled(true);
    if (_queue_pos == _queue.size()-1)
      _redo_action->setEnabled(false);
    emit redraw();
  }
}

