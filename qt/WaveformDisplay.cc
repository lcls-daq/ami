#include "WaveformDisplay.hh"

#include "ami/qt/QtBase.hh"
#include "ami/qt/AxisBins.hh"
#include "ami/qt/AxisControl.hh"
#include "ami/qt/Transform.hh"
#include "ami/qt/Cursors.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/PlotFrame.hh"
#include "ami/qt/PrintAction.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/DescTH1F.hh"
#include "ami/data/DescWaveform.hh"
#include "ami/data/Entry.hh"

#include <QtGui/QMenuBar>
#include <QtGui/QActionGroup>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QFileDialog>
#include <QtGui/QInputDialog>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

#include "qwt_plot_canvas.h"
#include "qwt_scale_engine.h"
#include "qwt_scale_draw.h"
#include "qwt_scale_widget.h"

#include <sys/uio.h>
#include <time.h>
#include <fstream>

using namespace Ami::Qt;

static const double no_scale[] = {0, 1000};

Ami::Qt::WaveformDisplay::WaveformDisplay() :
  QWidget(0)
{
  _plot = new PlotFrame(this);
  _plot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLinearScaleEngine);
  _plot->setAxisScaleEngine(QwtPlot::yLeft  , new QwtLinearScaleEngine);

  _xbins = new AxisBins(0,1000,10);
  _xinfo = _xbins;
  _yinfo = 0;

  _xrange = new AxisControl(this,"X");
  _yrange = new AxisControl(this,"Y");

  QPushButton* xtransB = new QPushButton("X Transform");
  _xtransform = new Transform(this, "X Transform","x");

  QMenuBar* menu_bar = new QMenuBar(this);
  {
    QMenu* file_menu = new QMenu("File");
    file_menu->addAction("Save image"     , this, SLOT(save_image()));
    file_menu->addAction("Save data"      , this, SLOT(save_data ()));
    file_menu->addAction("Save reference" , this, SLOT(save_reference()));
    file_menu->addSeparator();
    menu_bar->addMenu(file_menu);
    menu_bar->addAction(new PrintAction(*this));
  }

  QVBoxLayout* layout = new QVBoxLayout;
  { QGroupBox* plotBox = new QGroupBox("Plot");
    QVBoxLayout* layout1 = new QVBoxLayout;
    layout1->addWidget(menu_bar);
    layout1->addWidget(_plot);
    plotBox->setLayout(layout1);
    layout->addWidget(plotBox); }
  { QGridLayout* layout2 = new QGridLayout;
    layout2->addWidget(_xrange,0,0);
    layout2->addWidget(_yrange,0,1);
    { QHBoxLayout* layout3 = new QHBoxLayout;
      layout3->addStretch();
      layout3->addWidget(xtransB);
      layout3->addStretch();
      layout2->addLayout(layout3,1,0); }
    layout->addLayout(layout2); }
  setLayout(layout);

  connect(this   , SIGNAL(redraw()) , _plot      , SLOT(replot()));
  connect(_xrange, SIGNAL(windowChanged()), this , SLOT(xrange_change()));
  connect(_yrange, SIGNAL(windowChanged()), this , SLOT(yrange_change()));
  connect(xtransB, SIGNAL(clicked()), _xtransform, SLOT(show()));
  connect(_xtransform, SIGNAL(changed()), this, SLOT(xtransform_update()));
}

Ami::Qt::WaveformDisplay::~WaveformDisplay()
{
  delete _xbins;
}

void Ami::Qt::WaveformDisplay::save(char*& p) const
{
  _xtransform->save(p);
  _xrange->save(p);
  _yrange->save(p);
}

void Ami::Qt::WaveformDisplay::load(const char*& p)
{
  _xtransform->load(p);
  _xrange->load(p);
  _yrange->load(p);
}

void Ami::Qt::WaveformDisplay::save_image()
{
  char time_buffer[32];
  time_t seq_tm = time(NULL);
  strftime(time_buffer,32,"%Y%m%d_%H%M%S",localtime(&seq_tm));

  QString def("ami");
  def += "_";
  def += time_buffer;
  def += ".bmp";
  QString fname =
    QFileDialog::getSaveFileName(this,"Save File As (.bmp,.jpg,.png)",
                                 def,".bmp;.png;.jpg");
  if (!fname.isNull()) {
    QPixmap pixmap(QWidget::size());
    QWidget::render(&pixmap);
    pixmap.toImage().save(fname);
  }
}

void Ami::Qt::WaveformDisplay::save_data()
{
  if (!_curves.size())
    QMessageBox::warning(this, "Save data",
			 QString("No data to save"));
  else {

    char time_buffer[32];
    time_t seq_tm = time(NULL);
    strftime(time_buffer,32,"%Y%m%d_%H%M%S",localtime(&seq_tm));

    QString def("ami");
    def += "_";
    def += time_buffer;
    def += ".dat";
    QString fname =
      QFileDialog::getSaveFileName(this,"Save File As (.dat)",
				   def,".dat");
    if (!fname.isNull()) {
      FILE* f = fopen(qPrintable(fname),"w");
      if (!f)
	QMessageBox::warning(this, "Save data",
			     QString("Error opening %1 for writing").arg(fname));
      else {
	for(std::list<QtBase*>::const_iterator it=_curves.begin(); it!=_curves.end(); it++) {
	  (*it)->dump(f);
	  fprintf(f,"\n");
	}
	fclose(f);
      }
    }
  }
}

void Ami::Qt::WaveformDisplay::save_reference()
{
  QStringList list;
  for(std::list<QtBase*>::const_iterator it=_curves.begin(); it!=_curves.end(); it++) 
    list << (*it)->title();
  
  bool ok;
  QString choice = QInputDialog::getItem(this,"Reference Channel","Input",list,0,false,&ok);
  if (!ok) return;
  
  QtBase* ref = 0;
  for(std::list<QtBase*>::const_iterator it=_curves.begin(); it!=_curves.end(); it++) 
    if ((*it)->title()==choice) {
      ref = (*it);
      break;
    }

  if (ref==0) {
    printf("Reference %s not found\n",qPrintable(choice));
    return;
  }

  FILE* f = Path::saveReferenceFile(ref->title());
  if (f) {
    fwrite(&ref->entry().desc(), ref->entry().desc().size(), 1, f);
    iovec iov;  ref->entry().payload(iov);
    fwrite(iov.iov_base, iov.iov_len, 1, f);
    fclose(f);
  }
}
	  
void Ami::Qt::WaveformDisplay::prototype(const Ami::DescEntry* e)
{
  switch(e->type()) {
  case Ami::DescEntry::TH1F:
    { const Ami::DescTH1F& d = *static_cast<const Ami::DescTH1F*>(e);
      _xbins->update(d.xlow(),d.xup(),d.nbins());
      break; }
  case Ami::DescEntry::Waveform:
    { const Ami::DescWaveform& d = *static_cast<const Ami::DescWaveform*>(e);
      _xbins->update(d.xlow(),d.xup(),d.nbins());
      break; }
  default:
    break;
  }

  _xrange->update(*_xinfo);
}

void Ami::Qt::WaveformDisplay::add   (QtBase* b) 
{
  _xrange->update(*(_xinfo = b->xinfo()));
  _curves.push_back(b);
  b->xscale_update();
  b->update();
  b->attach(_plot);

  if (_xrange->isAuto()) {
  }

  emit redraw();
}

void Ami::Qt::WaveformDisplay::show(QtBase* b)
{
  for(std::list<QtBase*>::iterator it=_hidden.begin(); it!=_hidden.end(); it++) {
    if ((*it)==b) {
      _hidden.remove(b);
      _curves.push_back(b);

      b->xscale_update();
      b->update();
      b->attach(_plot);
      
      emit redraw();
      break;
    }
  }
}

void Ami::Qt::WaveformDisplay::hide(QtBase* b)
{
  for(std::list<QtBase*>::iterator it=_curves.begin(); it!=_curves.end(); it++) {
    if ((*it)==b) {
      _curves.remove(b);
      _hidden.push_back(b);
      
      b->attach((QwtPlot*)NULL);
      
      emit redraw();
      break;
    }
  }
}

void Ami::Qt::WaveformDisplay::reset()
{
  _curves.merge(_hidden);

  for(std::list<QtBase*>::iterator it=_curves.begin(); it!=_curves.end(); it++) {
    delete (*it);
  }
  _curves.clear();
}

void Ami::Qt::WaveformDisplay::update()
{
  for(std::list<QtBase*>::iterator it=_curves.begin(); it!=_curves.end(); it++)
    (*it)->update();

  emit redraw();
}

void Ami::Qt::WaveformDisplay::xtransform_update()
{
  for(std::list<QtBase*>::iterator it=_curves.begin(); it!=_curves.end(); it++)
    (*it)->xscale_update();

  if (_curves.size())
    _xrange->update(*(_xinfo=_curves.front()->xinfo()));

  if (_xrange->isAuto()) {
  }

  emit redraw();
}

void Ami::Qt::WaveformDisplay::xrange_change()
{
  if (_xrange->isAuto()) 
    _plot->setAxisAutoScale  (QwtPlot::xBottom);
  else
    _plot->setAxisScale      (QwtPlot::xBottom, _xrange->loEdge(), _xrange->hiEdge());

  if (_xrange->isLog())
    _plot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLog10ScaleEngine);
  else
    _plot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLinearScaleEngine);
}

void Ami::Qt::WaveformDisplay::yrange_change()
{
  if (_yrange->isAuto()) 
    _plot->setAxisAutoScale  (QwtPlot::yLeft);
  else
    _plot->setAxisScale      (QwtPlot::yLeft, _yrange->loEdge(), _yrange->hiEdge());

  if (_yrange->isLog())
    _plot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLog10ScaleEngine);
  else
    _plot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine);
}

const Ami::AbsTransform& Ami::Qt::WaveformDisplay::xtransform() const { return *_xtransform; }

const std::list<QtBase*> Ami::Qt::WaveformDisplay::plots() const { return _curves; }

const AxisInfo& Ami::Qt::WaveformDisplay::xinfo() const { return *_xinfo; }

PlotFrame* Ami::Qt::WaveformDisplay::plot() const { return _plot; }


