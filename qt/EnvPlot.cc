#include "EnvPlot.hh"

#include "ami/qt/AxisArray.hh"
#include "ami/qt/PlotFactory.hh"
#include "ami/qt/QtTH1F.hh"
#include "ami/qt/QtChart.hh"
#include "ami/qt/QtProf.hh"
#include "ami/qt/Path.hh"

#include "ami/data/AbsTransform.hh"
#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EnvPlot.hh"
#include "ami/data/RawFilter.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
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

EnvPlot::EnvPlot(QWidget*         parent,
		 const QString&   name,
		 DescEntry*       desc,
		 int              index0,
		 int              index1) :
  QtPWidget(parent),
  _name    (name),
  _desc    (desc),
  _index0  (index0),
  _index1  (index1),
  _output_signature  (0),
  _frame   (new QwtPlot(name)),
  _plot    (0),
  _counts  (new QLabel("Np 0"))
{
  _layout();
  show();
  connect(this, SIGNAL(redraw()), _frame, SLOT(replot()));
}

EnvPlot::EnvPlot(QWidget*     parent,
		 const char*& p) :
  QtPWidget(parent),
  _output_signature(0),
  _plot    (0),
  _counts  (new QLabel("Np 0"))
{
  load(p);

  _frame = new QwtPlot(_name);
  _layout();
  
  show();
  connect(this, SIGNAL(redraw()), _frame, SLOT(replot()));
}

EnvPlot::~EnvPlot()
{
  delete _desc;
  if (_plot    ) delete _plot;
}

void EnvPlot::_layout()
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

void EnvPlot::save(char*& p) const
{
  QtPWidget::save(p);

  QtPersistent::insert(p,_name);
  memcpy(p, _desc, _desc->size()); p += _desc->size();
  QtPersistent::insert(p,_index0);
  QtPersistent::insert(p,_index1);
}

#define CASEENTRY(type) case DescEntry::type: _desc = new Desc##type(*static_cast<Desc##type*>(desc)); break;

void EnvPlot::load(const char*& p)
{
  QtPWidget::load(p);

  _name = QtPersistent::extract_s(p);

  char* buff = new char[sizeof(DescProf)];
  DescEntry* desc = (DescEntry*)buff;
  memcpy(buff, p, sizeof(DescEntry));
  memcpy(buff+sizeof(DescEntry), p+sizeof(DescEntry), desc->size()-sizeof(DescEntry));
  switch(desc->type()) {
    CASEENTRY(TH1F)
    CASEENTRY(Prof)
    CASEENTRY(Scalar)
    default: break;
  }
  delete[] buff;

  _index0 = QtPersistent::extract_i(p);
  _index1 = QtPersistent::extract_i(p);
}

void EnvPlot::save_data()
{
  FILE* f = Path::saveDataFile();
  if (f) {
    _plot->dump(f);
    fclose(f);
  }
}

void EnvPlot::set_plot_title()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("Plot Title"), tr("Enter new title:"), 
				       QLineEdit::Normal, _frame->title().text(), &ok);
  if (ok)
    _frame->setTitle(text);
}

void EnvPlot::set_xaxis_title()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("X-Axis Title"), tr("Enter new title:"), 
				       QLineEdit::Normal, _frame->axisTitle(QwtPlot::xBottom).text(), &ok);
  if (ok)
    _frame->setAxisTitle(QwtPlot::xBottom,text);
}

void EnvPlot::set_yaxis_title()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("Y-Axis Title"), tr("Enter new title:"), 
				       QLineEdit::Normal, _frame->axisTitle(QwtPlot::yLeft).text(), &ok);
  if (ok)
    _frame->setAxisTitle(QwtPlot::yLeft,text);
}

#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

void EnvPlot::setup_payload(Cds& cds)
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
      printf("EnvPlot type %d not implemented yet\n",entry->desc().type()); 
      return;
    }
    _plot->attach(_frame);
    printf("%s found signature %d created type %d\n",qPrintable(_name),_output_signature,entry->desc().type());
  }
  else
    printf("%s output_signature %d not found\n",qPrintable(_name),_output_signature);
}

void EnvPlot::configure(char*& p, unsigned input, unsigned& output)
{
  Ami::EnvPlot op(*_desc, _index0, _index1);
  
  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						  ConfigureRequest::Discovery,
						  input,
						  _output_signature = ++output,
						  RawFilter(), op);
  p += r.size();
}

void EnvPlot::update()
{
  if (_plot) {
    _plot->update();
    _counts->setText(QString("Np %1").arg(_plot->normalization()));
    emit redraw();
  }
}
