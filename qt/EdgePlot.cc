#include "EdgePlot.hh"

#include "ami/qt/AxisArray.hh"
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

EdgePlot::EdgePlot(const QString&   name,
		   Ami::EdgeFinder* finder) :
  QWidget  (0),
  _name    (name),
  _finder  (finder),
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

EdgePlot::~EdgePlot()
{
  delete _finder;
  if (_plot    ) delete _plot;
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
			   const AxisArray& xinfo)
{
  unsigned channel = _finder->input();
  unsigned input_signature = signatures[channel];

  Ami::EdgeFinder op(input_signature, *_finder);

  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						  input,
						  _output_signature = ++output,
						  *channels[channel]->filter().filter(),
						  op);
  p += r.size();
}

void EdgePlot::update()
{
  if (_plot) {
    _plot->update();
    emit redraw();
  }
}
