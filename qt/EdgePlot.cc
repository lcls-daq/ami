#include "EdgePlot.hh"

#include "ami/qt/AxisInfo.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/PlotFactory.hh"
#include "ami/qt/QtTH1F.hh"
#include "ami/qt/Path.hh"

#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/AbsTransform.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EdgeFinder.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QMenuBar>
#include <QtGui/QActionGroup>
#include <QtGui/QInputDialog>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>

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

EdgePlot::EdgePlot(QWidget*         parent,
		   const QString&   name,
		   unsigned         channel,
		   Ami::EdgeFinder* finder) :
  QtPWidget(parent),
  _name    (name),
  _channel (channel),
  _finder  (finder),
  _frame   (new QwtPlot(name)),
  _plot    (0),
  _counts  (new QLabel("Np 0"))
{
  _layout();
  show();
  connect(this, SIGNAL(redraw()), _frame, SLOT(replot()));
}

EdgePlot::EdgePlot(QWidget* parent,
		   const char*& p) :
  QtPWidget(parent),
  _finder  (0),
  _plot    (0),
  _counts  (new QLabel("Np 0"))
{
  load(p);
  _layout();
  show();
  connect(this, SIGNAL(redraw()), _frame, SLOT(replot()));
  QtPWidget::load(p);
}

EdgePlot::~EdgePlot()
{
  delete _finder;
  if (_plot    ) delete _plot;
}

void EdgePlot::_layout()
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
      menu_bar->addMenu(annotate); }
    l->addWidget(menu_bar);
    l->addStretch();
    l->addWidget(_counts);
    layout->addLayout(l); }
  layout->addWidget(_frame);
  setLayout(layout);
}

void EdgePlot::save(char*& p) const
{
  QtPersistent::insert(p,_name);
  QtPersistent::insert(p,(int)_channel);

  p = (char*)_finder->serialize(p);
  
  QtPWidget::save(p);
}

void EdgePlot::load(const char*& p) 
{
  _name    = QtPersistent::extract_s(p);
  _channel = QtPersistent::extract_i(p);

  if (_finder) delete _finder;

  p += 2*sizeof(uint32_t);
  _finder = new Ami::EdgeFinder(p);
  _frame  = new QwtPlot(_name);

  _plot   = 0;
}

void EdgePlot::save_data()
{
  FILE* f = Path::saveDataFile();
  if (f) {
    _plot->dump(f);
    fclose(f);
  }
}

void EdgePlot::set_plot_title()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("Plot Title"), tr("Enter new title:"), 
				       QLineEdit::Normal, _frame->title().text(), &ok);
  if (ok)
    _frame->setTitle(text);
}

void EdgePlot::set_xaxis_title()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("X-Axis Title"), tr("Enter new title:"), 
				       QLineEdit::Normal, _frame->axisTitle(QwtPlot::xBottom).text(), &ok);
  if (ok)
    _frame->setAxisTitle(QwtPlot::xBottom,text);
}

void EdgePlot::set_yaxis_title()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("Y-Axis Title"), tr("Enter new title:"), 
				       QLineEdit::Normal, _frame->axisTitle(QwtPlot::yLeft).text(), &ok);
  if (ok)
    _frame->setAxisTitle(QwtPlot::yLeft,text);
}

#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

void EdgePlot::setup_payload(Cds& cds)
{
  if (_plot) delete _plot;
    
  Ami::Entry* entry = cds.entry(_output_signature);
  if (entry) {
    _plot = new QtTH1F(_name,*static_cast<const Ami::EntryTH1F*>(entry),
		       noTransform,noTransform,QColor(0,0,0));
    _plot->attach(_frame);
    printf("%s found signature %d created type %d\n",qPrintable(_name),_output_signature,entry->desc().type());
  }
  else
    printf("%s output_signature %d not found\n",qPrintable(_name),_output_signature);
}

void EdgePlot::configure(char*& p, unsigned input, unsigned& output,
			   ChannelDefinition* channels[], int* signatures, unsigned nchannels,
			   const AxisInfo& xinfo)
{
  unsigned input_signature = signatures[_channel];

  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						  ConfigureRequest::Analysis,
						  input_signature,
						  _output_signature = ++output,
						  *channels[_channel]->filter().filter(),
						  *_finder);
  p += r.size();
}

void EdgePlot::update()
{
  if (_plot) {
    _plot->update();
    _counts->setText(QString("Np %1").arg(_plot->normalization()));
    emit redraw();
  }
}
