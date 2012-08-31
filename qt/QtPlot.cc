#include "QtPlot.hh"

#include "ami/qt/AxisControl.hh"
#include "ami/qt/Defaults.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/PWidgetManager.hh"
#include "ami/qt/RunMaster.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QMenuBar>
#include <QtGui/QActionGroup>
#include <QtGui/QInputDialog>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_symbol.h"
#include "qwt_scale_engine.h"

using namespace Ami::Qt;

QtPlot::QtPlot(QWidget* parent,
	       const QString&   name) :
  QtPWidget(0),
  _name    (name),
  _frame   (new QwtPlot(name)),
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
  _grid->setMajPen(QPen(QColor(0x808080)));
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
}

QtPlot::~QtPlot()
{
  PWidgetManager::remove(this);
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
      annotate->addAction("Set Plot Style",        this, SLOT(set_style()));
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
      _frame = new QwtPlot(_name);
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

void QtPlot::set_style()
{
  _style.query(this);

  QColor qcol(0,0,0);
  QBrush qbrush(qcol);
  QPen   qpen(qcol); 
  qpen.setStyle((::Qt::PenStyle)_style.line_style());
  qpen.setWidth(_style.line_size());
  QSize  qsize(_style.symbol_size(),_style.symbol_size());

  QwtPlotCurve* item = 0;
  const QwtPlotItemList& list = _frame->itemList();

  for(int i=0; i<list.size(); i++) {
    item = dynamic_cast<QwtPlotCurve*>(list.at(i));
    if (!item) continue;

    QwtSymbol s((QwtSymbol::Style)_style.symbol_style(),
                qbrush, 
                qpen, 
                qsize);
    item->setSymbol(s);
    item->setPen(qpen);
  }
  emit redraw();
}
