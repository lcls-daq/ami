#include "QtPlot.hh"

#include "ami/qt/AxisControl.hh"
#include "ami/qt/Defaults.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/PWidgetManager.hh"
#include "ami/qt/RunMaster.hh"
#include "ami/qt/QtPlotSelector.hh"
#include "ami/qt/QtOverlay.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QMenuBar>
#include <QtGui/QActionGroup>
#include <QtGui/QInputDialog>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPainter>

#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_symbol.h"
#include "qwt_scale_engine.h"

namespace Ami {
  namespace Qt {
    class QwtLPlot : public QwtPlot {
    public:
      QwtLPlot(const QString& name) : QwtPlot(name) { setAutoDelete(false); }
      ~QwtLPlot() {}
    public:
      //
      //  Draw a legend if more than one QwtPlotCurve is shown
      //
      void drawItems(QPainter*     p,
                     const QRect&  canvasRect,
                     const QwtScaleMap map[axisCnt],
                     const QwtPlotPrintFilter &pfilter ) const 
      {
        QwtPlot::drawItems(p, canvasRect, map, pfilter);

        const QwtPlotItemList& list = itemList();
        int row=0;
        for(int i=0; i<list.size(); i++)
          if (dynamic_cast<QwtPlotCurve*>(list[i]))
            if (row++) break;

        if (row>1) {
          row=0;
          int width = canvasRect.width();
          QPainter& painter = *p;
          for(int i=0; i<list.size(); i++) {
            QwtPlotCurve* c = dynamic_cast<QwtPlotCurve*>(list[i]);
            if (c) {
              painter.setPen  (c->pen().color());
              const QString& title = c->title().text();
              if (width > 20) {
                painter.drawText(10, 18*row, width-10, 18, ::Qt::AlignLeft, 
                                 title.mid(0,title.indexOf('#')));
                row++;
              }
            }
          }
        }
      }
    };
  };
};

using namespace Ami::Qt;

static std::vector<QtPlot*> _plots;
static QStringList          _scalar_plots;
static QStringList          _1d_plots;
static QStringList          _2d_plots;
static QStringList          _no_plots;
static QStringList*         _plot_names[] = { &_scalar_plots,  // Scalar [Chart]
                                              &_1d_plots,      // TH1F
                                              &_no_plots,      // cant overlap TH2F
                                              &_2d_plots,      // Prof
                                              &_no_plots,      // cant overlap Image
                                              &_2d_plots,      // Waveform
                                              &_2d_plots,      // Scan
                                              &_no_plots,      // Ref
                                              &_no_plots,      // Cache
                                              &_no_plots,      // ScalarRange
                                              &_no_plots };    // ScalarDRange
static QtPlotSelector*      _selector=0;

QtPlot::QtPlot(QWidget* parent,
	       const QString&   name) :
  QtPWidget(0),
  _name    (name),
  _frame   (new QwtLPlot(name)),
  _runnum  (new QLabel("")),
  _counts  (new QLabel("Np 0")),
  _xrange  (new AxisControl(this,"X")),
  _yrange  (new AxisControl(this,"Y")),
  _grid    (new QwtPlotGrid)
{
  PWidgetManager::add(this, _name);

  bool gMajor = Defaults::instance()->show_grid();
  _grid->enableX   (gMajor);
  _grid->enableY   (gMajor);
  bool gMinor = Defaults::instance()->show_minor_grid();
  _grid->enableXMin(gMinor);
  _grid->enableYMin(gMinor);
  _grid->setMajPen(QPen(QColor(0xa0a0a0)));
  _grid->setMinPen(QPen(QColor(0xc0c0c0)));
  _grid->attach(_frame);

  _layout();
}

QtPlot::QtPlot(QWidget* parent) :
  QtPWidget(0),
  _runnum  (new QLabel("")),
  _counts  (new QLabel("Np 0")),
  _xrange  (new AxisControl(this,"X")),
  _yrange  (new AxisControl(this,"Y")),
  _grid    (new QwtPlotGrid)
{
  _grid->setMajPen(QPen(QColor(0xa0a0a0)));
  _grid->setMinPen(QPen(QColor(0xc0c0c0)));
}

QtPlot::~QtPlot()
{
  PWidgetManager::remove(this);
  _remove(this);

  if (_grid) delete _grid;

  for(std::list<QtOverlay*>::iterator it=_ovls.begin();
      it != _ovls.end(); it++) {
    QtOverlay* o = *it;
    delete o;
  }
}

void QtPlot::_layout()
{
  setAttribute(::Qt::WA_DeleteOnClose, true);
  setWindowTitle(_name);
  
  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* l = new QHBoxLayout;
    QMenuBar* menu_bar = new QMenuBar;
    { QMenu* file_menu = new QMenu("File");
      file_menu->addAction("Save data", this, SLOT(save_data()));
      menu_bar->addMenu(file_menu); }
    { QMenu* annotate = new QMenu("Annotate");
      annotate->addAction("Plot Title"           , this, SLOT(set_plot_title()));
      annotate->addAction("Y-axis Title (left)"  , this, SLOT(set_yaxis_title()));
      annotate->addAction("X-axis Title (bottom)", this, SLOT(set_xaxis_title()));
      annotate->addAction("Toggle Grid"          , this, SLOT(toggle_grid()));
      annotate->addAction("Toggle Minor Grid"    , this, SLOT(toggle_minor_grid()));
      annotate->addAction("Set Plot Style",        this, SLOT(query_style()));
      menu_bar->addMenu(annotate); }
    l->addWidget(menu_bar);
    l->addStretch();
    l->addWidget(_runnum); 
    l->addStretch();
    l->addWidget(_counts); 
    layout->addLayout(l); }
  layout->addWidget(_frame);
  layout->setSpacing(0);
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(_xrange);
    layout1->addStretch();
    layout->addLayout(layout1); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(_yrange);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);
  
  show();
  connect(this, SIGNAL(redraw()), _frame, SLOT(replot()));
  connect(this, SIGNAL(counts_changed(double)), 
	  this, SLOT(update_counts(double)));
  connect(_xrange, SIGNAL(windowChanged()), this , SLOT(xrange_change()));
  connect(_yrange, SIGNAL(windowChanged()), this , SLOT(yrange_change()));
}

void QtPlot::save(char*& p) const
{
  XML_insert(p, "QString", "_name", QtPersistent::insert(p,_name) );
  XML_insert(p, "QString", "_frame_title", QtPersistent::insert(p,_frame->title().text()) );
  XML_insert(p, "QString", "_frame_xtitle", QtPersistent::insert(p,_frame->axisTitle(QwtPlot::xBottom).text()) );
  XML_insert(p, "QString", "_frame_ytitle", QtPersistent::insert(p,_frame->axisTitle(QwtPlot::yLeft).text()) );
  //  _xrange->save(p);
  XML_insert(p, "AxisControl", "_yrange", _yrange->save(p) );
  XML_insert(p, "bool", "_grid_xenabled"   , QtPersistent::insert(p,_grid->xEnabled()) );
  XML_insert(p, "bool", "_grid_xminenabled", QtPersistent::insert(p,_grid->xMinEnabled()) );
  XML_insert(p, "QtPlotStyle", "_style", _style.save(p) );
  XML_insert(p, "QtPWidget", "self", QtPWidget::save(p) );
}

void QtPlot::load(const char*& p)
{
  XML_iterate_open(p,tag)

    if (tag.name == "_name") {
      _name  = QtPersistent::extract_s(p);
      _frame = new QwtLPlot(_name);
      _grid->attach(_frame);
    }
    else if (tag.name == "_frame_title")
      _frame->setTitle(QtPersistent::extract_s(p));
    else if (tag.name == "_frame_xtitle")
      _frame->setAxisTitle(QwtPlot::xBottom,QtPersistent::extract_s(p));
    else if (tag.name == "_frame_ytitle")
      _frame->setAxisTitle(QwtPlot::yLeft  ,QtPersistent::extract_s(p));
    else if (tag.name == "_yrange")
      _yrange->load(p);
    else if (tag.name == "_grid_xenabled") {
      bool gMajor = QtPersistent::extract_b(p);
      _grid->enableX   (gMajor);
      _grid->enableY   (gMajor);
    }
    else if (tag.name == "_grid_xminenabled") {
      bool gMinor = QtPersistent::extract_b(p);
      _grid->enableXMin(gMinor);
      _grid->enableYMin(gMinor);
    }
    else if (tag.name == "_style")
      _style.load(p);
    else if (tag.element == "QtPWidget")
      QtPWidget::load(p);

  XML_iterate_close(QtPlot,tag);

  PWidgetManager::add(this, _name);
  _layout();
}

void QtPlot::save_data()
{
  FILE* f = Path::saveDataFile(this);
  if (f) {
    dump(f);
    fclose(f);
  }
}

void QtPlot::set_plot_title()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("Plot Title"), tr("Enter new title:"), 
				       QLineEdit::Normal, _frame->title().text(), &ok);
  if (ok) {
    _frame->setTitle(text);
    _name = text;
    setWindowTitle(text);
    PWidgetManager::remove(this);
    PWidgetManager::add(this,text);
  }
}

void QtPlot::set_xaxis_title()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("X-Axis Title"), tr("Enter new title:"), 
				       QLineEdit::Normal, _frame->axisTitle(QwtPlot::xBottom).text(), &ok);
  if (ok)
    _frame->setAxisTitle(QwtPlot::xBottom,text);
}

void QtPlot::set_yaxis_title()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("Y-Axis Title"), tr("Enter new title:"), 
				       QLineEdit::Normal, _frame->axisTitle(QwtPlot::yLeft).text(), &ok);
  if (ok)
    _frame->setAxisTitle(QwtPlot::yLeft,text);
}

void QtPlot::toggle_grid()
{
  bool gEnable = !_grid->xEnabled();
  _grid->enableX(gEnable);
  _grid->enableY(gEnable);
  emit redraw();
}

void QtPlot::toggle_minor_grid()
{
  bool gEnable = !_grid->xMinEnabled();
  _grid->enableXMin(gEnable);
  _grid->enableYMin(gEnable);
  emit redraw();
}

void QtPlot::edit_xrange(bool v)
{
  _xrange->setVisible(v);
}

void QtPlot::edit_yrange(bool v)
{
  _yrange->setVisible(v);
}

void QtPlot::xrange_change()
{
  if (_xrange->isAuto())
    _frame->setAxisAutoScale  (QwtPlot::xBottom);
  else
    _frame->setAxisScale      (QwtPlot::xBottom, _xrange->loEdge(), _xrange->hiEdge());

  if (_xrange->isLog())
    _frame->setAxisScaleEngine(QwtPlot::xBottom, new QwtLog10ScaleEngine);
  else
    _frame->setAxisScaleEngine(QwtPlot::xBottom, new QwtLinearScaleEngine);
}

void QtPlot::yrange_change()
{
  if (_yrange->isAuto())
    _frame->setAxisAutoScale  (QwtPlot::yLeft);
  else
    _frame->setAxisScale      (QwtPlot::yLeft, _yrange->loEdge(), _yrange->hiEdge());

  if (_yrange->isLog())
    _frame->setAxisScaleEngine(QwtPlot::yLeft, new QwtLog10ScaleEngine);
  else
    _frame->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine);
}

void QtPlot::update_counts(double n)
{
  _runnum->setText(RunMaster::instance()->run_title());
  _counts->setText(QString("Np %1").arg(n));
}

void QtPlot::query_style()
{
  _style.query(this);
  set_style();
}

void QtPlot::set_style()
{
  QSize  qsize(_style.symbol_size(),_style.symbol_size());

  QwtPlotCurve* item = 0;
  const QwtPlotItemList& list = _frame->itemList();

  for(int i=0; i<list.size(); i++) {
    item = dynamic_cast<QwtPlotCurve*>(list.at(i));
    if (!item) continue;

    QPen qpen(item->pen());
    qpen.setStyle((::Qt::PenStyle)_style.line_style());
    qpen.setWidth(_style.line_size());
    item->setPen(qpen);

    QwtSymbol s(item->symbol());
    s.setStyle((QwtSymbol::Style)_style.symbol_style());
    s.setPen  (qpen);
    s.setSize (qsize);
    item->setSymbol(s);
  }
  emit redraw();
}


const QStringList& QtPlot::names(Ami::DescEntry::Type t) 
{
  return *_plot_names[t]; 
}

QtPlot* QtPlot::lookup(const QString& name)
{
  for(int i=0; i<(int)_plots.size(); i++)
    if (_plots[i]->_name == name)
      return _plots[i];
  return 0;
}

void QtPlot::_remove(QtPlot* p)
{
  for(unsigned i=0; i<_plots.size(); i++)
    if (_plots[i]==p) {
      _plots.erase(_plots.begin()+i);
      _plot_names[p->_type]->removeAll(p->_name);
      return;
    }
}

void QtPlot::select(QtPlotSelector* s) { _selector=s; }

void QtPlot::mousePressEvent(QMouseEvent* e)
{
  if (_selector && _plot_names[_selector->type()]->contains(_name))
    _selector->plot_selected(this);

  QtPWidget::mousePressEvent(e);
}

void QtPlot::add_overlay(QtOverlay* o)
{
  _ovls.push_back(o);
}

void QtPlot::setPlotType(Ami::DescEntry::Type t)
{
  _type = t;
  _plot_names[t]->push_back(_name);
  _plots         .push_back(this);

  _style.setPlotType(t);
}
