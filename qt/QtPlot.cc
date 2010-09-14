#include "QtPlot.hh"

#include "ami/qt/AxisControl.hh"
#include "ami/qt/Defaults.hh"
#include "ami/qt/Path.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QMenuBar>
#include <QtGui/QActionGroup>
#include <QtGui/QInputDialog>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

#include "qwt_plot.h"
#include "qwt_plot_grid.h"
#include "qwt_scale_engine.h"

using namespace Ami::Qt;

QtPlot::QtPlot(QWidget* parent,
	       const QString&   name) :
  QtPWidget(parent),
  _name    (name),
  _frame   (new QwtPlot(name)),
  _counts  (new QLabel("Np 0")),
  _xrange  (new AxisControl(this,"X")),
  _yrange  (new AxisControl(this,"Y")),
  _grid    (new QwtPlotGrid)
{
  _layout();

  bool gMajor = Defaults::instance()->show_grid();
  _grid->enableX   (gMajor);
  _grid->enableY   (gMajor);
  bool gMinor = Defaults::instance()->show_minor_grid();
  _grid->enableXMin(gMinor);
  _grid->enableYMin(gMinor);
  _grid->attach(_frame);
}

QtPlot::QtPlot(QWidget* parent,
	       const char*& p) :
  QtPWidget(parent),
  _counts  (new QLabel("Np 0")),
  _xrange  (new AxisControl(this,"X")),
  _yrange  (new AxisControl(this,"Y")),
  _grid    (new QwtPlotGrid)
{
  _name  = QtPersistent::extract_s(p);
  _frame = new QwtPlot(_name);
  _frame->setTitle(QtPersistent::extract_s(p));
  _frame->setAxisTitle(QwtPlot::xBottom,QtPersistent::extract_s(p));
  _frame->setAxisTitle(QwtPlot::yLeft  ,QtPersistent::extract_s(p));

  //  _xrange->load(p);
  _yrange->load(p);

  _layout();

  bool gMajor = QtPersistent::extract_b(p);
  _grid->enableX   (gMajor);
  _grid->enableY   (gMajor);
  bool gMinor = QtPersistent::extract_b(p);
  _grid->enableXMin(gMinor);
  _grid->enableYMin(gMinor);
  _grid->attach(_frame);

  QtPWidget::load(p);
}

QtPlot::~QtPlot()
{
}

void QtPlot::_layout()
{
  setAttribute(::Qt::WA_DeleteOnClose, true);
  
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
      menu_bar->addMenu(annotate); }
    l->addWidget(menu_bar);
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
  QtPersistent::insert(p,_name);
  QtPersistent::insert(p,_frame->title().text());
  QtPersistent::insert(p,_frame->axisTitle(QwtPlot::xBottom).text());
  QtPersistent::insert(p,_frame->axisTitle(QwtPlot::yLeft).text());
  //  _xrange->save(p);
  _yrange->save(p);
  QtPersistent::insert(p,_grid->xEnabled());
  QtPersistent::insert(p,_grid->xMinEnabled());
  QtPWidget::save(p);
}

void QtPlot::load(const char*& p)
{
}

void QtPlot::save_data()
{
  FILE* f = Path::saveDataFile();
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
  if (ok)
    _frame->setTitle(text);
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
  _counts->setText(QString("Np %1").arg(n));
}
