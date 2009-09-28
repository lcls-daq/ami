#include "CursorPlot.hh"

#include "ami/qt/AxisInfo.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/PlotFactory.hh"
#include "ami/qt/QtTH1F.hh"
#include "ami/qt/QtChart.hh"
#include "ami/qt/QtProf.hh"
#include "ami/qt/Path.hh"

#include "ami/data/BinMath.hh"
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

CursorPlot::CursorPlot(const QString&   name,
		       unsigned         channel,
		       BinMath*         input) :
  QWidget  (0),
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

CursorPlot::~CursorPlot()
{
  delete _input;
  if (_plot    ) delete _plot;
}

void CursorPlot::save_data()
{
  FILE* f = Path::saveDataFile();
  if (f) {
    _plot->dump(f);
    fclose(f);
  }
}

void CursorPlot::set_plot_title()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("Plot Title"), tr("Enter new title:"), 
				       QLineEdit::Normal, _frame->title().text(), &ok);
  if (ok)
    _frame->setTitle(text);
}

void CursorPlot::set_xaxis_title()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("X-Axis Title"), tr("Enter new title:"), 
				       QLineEdit::Normal, _frame->axisTitle(QwtPlot::xBottom).text(), &ok);
  if (ok)
    _frame->setAxisTitle(QwtPlot::xBottom,text);
}

void CursorPlot::set_yaxis_title()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("Y-Axis Title"), tr("Enter new title:"), 
				       QLineEdit::Normal, _frame->axisTitle(QwtPlot::yLeft).text(), &ok);
  if (ok)
    _frame->setAxisTitle(QwtPlot::yLeft,text);
}

#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

void CursorPlot::setup_payload(Cds& cds)
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
      printf("CursorPlot type %d not implemented yet\n",entry->desc().type()); 
      return;
    }
    _plot->attach(_frame);
    printf("%s found signature %d created type %d\n",qPrintable(_name),_output_signature,entry->desc().type());
  }
  else
    printf("%s output_signature %d not found\n",qPrintable(_name),_output_signature);
}

void CursorPlot::configure(char*& p, unsigned input, unsigned& output,
			   ChannelDefinition* channels[], int* signatures, unsigned nchannels,
			   const AxisInfo& xinfo, ConfigureRequest::Source source)
{
  unsigned channel = _channel;
  unsigned input_signature = signatures[channel];

  // replace cursor values with bin indices
  QString expr(_input->expression());
  QString new_expr;
  { QRegExp match("\\[[^\\]]*\\]");
    int last=0;
    int pos=0;
    while( (pos=match.indexIn(expr,pos)) != -1) {
      QString use = expr.mid(pos+1,match.matchedLength()-2);
      bool ok;
      double v = use.toDouble(&ok);
      unsigned bin=0;
      if (!ok)
	printf("error parsing double %s\n",qPrintable(use));
      else {
	bin = xinfo.tick(v);
      }
      new_expr.append(expr.mid(last,pos-last));
      new_expr.append(QString("[%1]").arg(bin));
      pos += match.matchedLength();
      last = pos;
    }
    new_expr.append(expr.mid(last));
    new_expr.replace(QString("]%1[").arg(BinMath::integrate()),QString(BinMath::integrate()));
    new_expr.replace(QString("]%1[").arg(BinMath::range    ()),QString(BinMath::range    ()));
  }
  QString end_expr;
  { int last=0, next=0, pos=0;
    while( (pos=new_expr.indexOf(BinMath::range(),pos)) != -1) {
      if ( (next=new_expr.lastIndexOf("[",pos))==-1 )
	printf("error parsing range in %s\n",qPrintable(expr));
      else {
	end_expr.append(new_expr.mid(last,next-last));
	last  = new_expr.indexOf("]",pos);
	int a = new_expr.mid(next+1,pos -next-1).toInt();
	int b = new_expr.mid(pos +1,last-pos -1).toInt();
	printf("%s/%d %s/%d\n",
	       qPrintable(new_expr.mid(next+1,pos -next-1)),a,
	       qPrintable(new_expr.mid(pos +1,last-pos -1)),b);
	end_expr.append(QString("(%1)").arg(QString::number(abs(a-b)+1)));
	pos  = ++last;
      }
    }
    end_expr.append(new_expr.mid(last));
  }
  printf("CursorPlot %s\n",qPrintable(expr));
  printf("CursorPlot %s\n",qPrintable(new_expr));
  printf("CursorPlot %s\n",qPrintable(end_expr));

  Ami::BinMath op(_input->output(), qPrintable(end_expr),
		  _input->feature_index());
  
  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						  source,
						  input_signature,
						  _output_signature = ++output,
						  *channels[channel]->filter().filter(),
						  op);
  p += r.size();
}

void CursorPlot::update()
{
  if (_plot) {
    _plot->update();
    emit redraw();
  }
}
