#include "PeakFitPlot.hh"

#include "ami/qt/AxisInfo.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/PlotFactory.hh"
#include "ami/qt/QtTH1F.hh"
#include "ami/qt/QtChart.hh"
#include "ami/qt/QtProf.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/FeatureRegistry.hh"

#include "ami/data/PeakFitPlot.hh"
#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/AbsTransform.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScalar.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QMenuBar>
#include <QtGui/QActionGroup>
#include <QtGui/QInputDialog>
#include <QtGui/QLineEdit>

#include "qwt_plot.h"

namespace Ami {
  namespace Qt {
    class NullTransform : public Ami::AbsTransform {
    public:
      ~NullTransform() {}
      double operator()(double x) const { return x; }
    };
  };
};

using namespace Ami::Qt;

static NullTransform noTransform;

PeakFitPlot::PeakFitPlot(QWidget* parent,
			 const QString&    name,
			 unsigned          channel,
			 Ami::PeakFitPlot* input) :
  QtPWidget(parent),
  _name    (name),
  _channel (channel),
  _input   (input),
  _output_signature  (0),
  _frame   (new QwtPlot(name)),
  _plot    (0)
{
  setAttribute(::Qt::WA_DeleteOnClose, true);
  
  QVBoxLayout* layout = new QVBoxLayout;
  QMenuBar* menu_bar = new QMenuBar;
  { QMenu* file_menu = new QMenu("File");
    file_menu->addAction("Save data", this, SLOT(save_data()));
    menu_bar->addMenu(file_menu); }
  { QMenu* annotate = new QMenu("Annotate");
    annotate->addAction("Plot Title"           , this, SLOT(set_plot_title()));
    annotate->addAction("Y-axis Title (left)"  , this, SLOT(set_yaxis_title()));
    annotate->addAction("X-axis Title (bottom)", this, SLOT(set_xaxis_title()));
    menu_bar->addMenu(annotate); }
  layout->addWidget(menu_bar);
  layout->addWidget(_frame);
  setLayout(layout);
  
  show();
  connect(this, SIGNAL(redraw()), _frame, SLOT(replot()));
}

PeakFitPlot::PeakFitPlot(QWidget* parent,
		       const char*& p) :
  QtPWidget(parent)
{
  load(p);

  setAttribute(::Qt::WA_DeleteOnClose, true);
  
  QVBoxLayout* layout = new QVBoxLayout;
  QMenuBar* menu_bar = new QMenuBar;
  { QMenu* file_menu = new QMenu("File");
    file_menu->addAction("Save data", this, SLOT(save_data()));
    menu_bar->addMenu(file_menu); }
  { QMenu* annotate = new QMenu("Annotate");
    annotate->addAction("Plot Title"           , this, SLOT(set_plot_title()));
    annotate->addAction("Y-axis Title (left)"  , this, SLOT(set_yaxis_title()));
    annotate->addAction("X-axis Title (bottom)", this, SLOT(set_xaxis_title()));
    menu_bar->addMenu(annotate); }
  layout->addWidget(menu_bar);
  layout->addWidget(_frame);
  setLayout(layout);
  
  show();
  connect(this, SIGNAL(redraw()), _frame, SLOT(replot()));

  QtPWidget::load(p);
}

PeakFitPlot::~PeakFitPlot()
{
  delete _input;
  if (_plot    ) delete _plot;
}

void PeakFitPlot::save(char*& p) const
{
  QtPersistent::insert(p,_name);
  QtPersistent::insert(p,(int)_channel);
  p = (char*)_input->serialize(p);
  QtPWidget::save(p);
}

void PeakFitPlot::load(const char*& p)
{
  _name    = QtPersistent::extract_s(p);
  _channel = QtPersistent::extract_i(p);

  p += 2*sizeof(uint32_t);
  _input = new Ami::PeakFitPlot(p);
  _output_signature=0;
  _frame = new QwtPlot(_name);
  _plot  = 0;
}

void PeakFitPlot::save_data()
{
  FILE* f = Path::saveDataFile();
  if (f) {
    _plot->dump(f);
    fclose(f);
  }
}

void PeakFitPlot::set_plot_title()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("Plot Title"), tr("Enter new title:"), 
				       QLineEdit::Normal, _frame->title().text(), &ok);
  if (ok)
    _frame->setTitle(text);
}

void PeakFitPlot::set_xaxis_title()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("X-Axis Title"), tr("Enter new title:"), 
				       QLineEdit::Normal, _frame->axisTitle(QwtPlot::xBottom).text(), &ok);
  if (ok)
    _frame->setAxisTitle(QwtPlot::xBottom,text);
}

void PeakFitPlot::set_yaxis_title()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("Y-Axis Title"), tr("Enter new title:"), 
				       QLineEdit::Normal, _frame->axisTitle(QwtPlot::yLeft).text(), &ok);
  if (ok)
    _frame->setAxisTitle(QwtPlot::yLeft,text);
}

#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

void PeakFitPlot::setup_payload(Cds& cds)
{
  if (_plot) delete _plot;
    
  Ami::Entry* entry = cds.entry(_output_signature);
  if (entry) {
    switch(entry->desc().type()) {
    case Ami::DescEntry::TH1F: 
      _plot = new QtTH1F(_name,*static_cast<const Ami::EntryTH1F*>(entry),
			 noTransform,noTransform,QColor(0,0,0));
      break;
    case Ami::DescEntry::Scalar:  // create a chart from a scalar
      _plot = new QtChart(_name,*static_cast<const Ami::EntryScalar*>(entry),
			  200,QColor(0,0,0));
      break;
    case Ami::DescEntry::Prof: 
      _plot = new QtProf(_name,*static_cast<const Ami::EntryProf*>(entry),
			 noTransform,noTransform,QColor(0,0,0));
      break;
    default:
      printf("PeakFitPlot type %d not implemented yet\n",entry->desc().type()); 
      return;
    }
    _plot->attach(_frame);
    printf("%s found signature %d created type %d\n",qPrintable(_name),_output_signature,entry->desc().type());
  }
  else
    printf("%s output_signature %d not found\n",qPrintable(_name),_output_signature);
}

void PeakFitPlot::configure(char*& p, unsigned input, unsigned& output,
			   ChannelDefinition* channels[], int* signatures, unsigned nchannels,
			   const AxisInfo& xinfo, ConfigureRequest::Source source)
{
  unsigned channel = _channel;
  unsigned input_signature = signatures[channel];

  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						  source,
						  input_signature,
						  _output_signature = ++output,
						  *channels[channel]->filter().filter(),
						  *_input);
  p += r.size();
}

void PeakFitPlot::update()
{
  if (_plot) {
    _plot->update();
    emit redraw();
  }
}
