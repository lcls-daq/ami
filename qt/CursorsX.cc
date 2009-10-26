#include "CursorsX.hh"

#include "ami/qt/DescTH1F.hh"
#include "ami/qt/DescProf.hh"
#include "ami/qt/DescScan.hh"
#include "ami/qt/DescChart.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/CursorDefinition.hh"
#include "ami/qt/CursorPlot.hh"
#include "ami/qt/WaveformDisplay.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/Calculator.hh"
#include "ami/qt/PlotFrame.hh"

#include "ami/data/DescScalar.hh"
#include "ami/data/DescTH1F.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/DescScan.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/Expression.hh"

#include "ami/data/Integral.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>
#include <QtGui/QComboBox>
#include <QtGui/QDoubleSpinBox>
#include <QtCore/QRegExp>
#include <QtGui/QRegExpValidator>
#include <QtGui/QMessageBox>

#include <sys/socket.h>

// works for labels not buttons
//#define bold(t) "<b>" #t "</b>"
#define bold(t) #t

enum { _TH1F, _vT, _vF, _vS };


namespace Ami {
  namespace Qt {
    class CursorLocation : public QLineEdit {
    public:
      CursorLocation() : QLineEdit("0") { new QDoubleValidator(this); }
      ~CursorLocation() {}
    public:
      double value() const { return text().toDouble(); }
    };
  };
};

using namespace Ami::Qt;

//static QChar _integrate   (0x2026);  // ..
static QChar _integrate   (0x222b);  // S
static QChar _range       (0x2194);  // left-right arrow
static QChar _exponentiate(0x005E);
static QChar _multiply    (0x00D7);
static QChar _divide      (0x00F7);
static QChar _add         (0x002B);
static QChar _subtract    (0x002D);

CursorsX::CursorsX(QWidget* parent, ChannelDefinition* channels[], unsigned nchannels, WaveformDisplay& frame) :
  QtPWidget (parent),
  Cursors   (*frame.plot()),
  _channels (channels),
  _nchannels(nchannels),
  _channel  (0),
  _frame    (frame),
  _clayout  (new QVBoxLayout),
  _expr     (new QLineEdit("1")),
  _title    (new QLineEdit("Cursor plot"))
{
  _names << "a" << "b" << "c" << "d" << "f" << "g" << "h" << "i" << "j" << "k";

  _expr->setReadOnly(true);

  _new_value = new CursorLocation;

  setWindowTitle("CursorsX Plot");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QComboBox* channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    channelBox->addItem(channels[i]->name());

  _hist   = new DescTH1F   (bold(Sum (1dH)));
  _vTime  = new DescChart  (bold(Mean v Time),0.2);
  _vFeature = new DescProf (bold(Mean v Var) );
  _vScan    = new DescScan (bold(Mean v Scan));

  _plot_grp = new QButtonGroup;
  _plot_grp->addButton(_hist    ->button(),_TH1F);
  _plot_grp->addButton(_vTime   ->button(),_vT);
  _plot_grp->addButton(_vFeature->button(),_vF);
  _plot_grp->addButton(_vScan   ->button(),_vS);
  _hist->button()->setChecked(true);

  QPushButton* calcB  = new QPushButton("Enter");
  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* closeB = new QPushButton("Close");
  QPushButton* grabB  = new QPushButton("Grab");
  
  QVBoxLayout* layout = new QVBoxLayout;
  { QGroupBox* channel_box = new QGroupBox("Source Channel");
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Channel"));
    layout1->addWidget(channelBox);
    layout1->addStretch();
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  { QGroupBox* locations_box = new QGroupBox("Define CursorsX");
    locations_box->setToolTip("Define a cursor with a NAME and x-axis LOCATION.");
    QVBoxLayout* layout2 = _clayout;
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addWidget(new QLabel("Location"));
      layout1->addWidget(_new_value);
      layout1->addWidget(grabB);
      layout2->addLayout(layout1); }
    locations_box->setLayout(layout2);
    layout->addWidget(locations_box); }
  { QGroupBox* expr_box = new QGroupBox("Expression:");
    expr_box->setToolTip(QString("Expression is a set of cursor names with the operations:\n" \
				 "  A %1 B : Integrate between cursors A and B\n"\
				 "  A %2 B : Count of bins between cursors A and B\n"\
				 "  A %3 B : Exponentiate [value at] A to the power of [value at] B\n "	\
				 "  A %4 B : Multiply [value at] A by [value at] B\n "	\
				 "  A %5 B : Divide \n "	\
				 "  A %6 B : Add \n "		\
				 "  A %7 B : Subtract \n ")
			 .arg(_integrate)
			 .arg(_range)
			 .arg(_exponentiate)
			 .arg(_multiply)
			 .arg(_divide)
			 .arg(_add)
			 .arg(_subtract));
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Expr"));
    layout1->addWidget(_expr);
    layout1->addWidget(calcB);
    expr_box->setLayout(layout1); 
    layout->addWidget(expr_box); }
  { QGroupBox* plot_box = new QGroupBox("Plot");
    QVBoxLayout* layout1 = new QVBoxLayout;
    { QHBoxLayout* layout2 = new QHBoxLayout;
      layout2->addWidget(new QLabel("Title"));
      layout2->addWidget(_title);
      layout1->addLayout(layout2); }
    layout1->addWidget(_hist );
    layout1->addWidget(_vTime);
    layout1->addWidget(_vFeature);
    layout1->addWidget(_vScan);
    plot_box->setLayout(layout1); 
    layout->addWidget(plot_box); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(plotB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);
    
  connect(channelBox, SIGNAL(activated(int)), this, SLOT(set_channel(int)));
  connect(_new_value, SIGNAL(returnPressed()),this, SLOT(add_cursor()));
  connect(calcB     , SIGNAL(clicked()),      this, SLOT(calc()));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));
  connect(grabB     , SIGNAL(clicked()),      this, SLOT(grab_cursorx()));
  connect(this      , SIGNAL(grabbed()),      this, SLOT(add_cursor()));
}
  
CursorsX::~CursorsX()
{
}

void CursorsX::save(char*& p) const
{
  QtPWidget::save(p);

  for(std::list<CursorDefinition*>::const_iterator it=_cursors.begin(); it!=_cursors.end(); it++) {
    QtPersistent::insert(p,QString("CursorDef"));
    QtPersistent::insert(p,(*it)->name());
    QtPersistent::insert(p,(*it)->location());
  }

  for(std::list<CursorPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    QtPersistent::insert(p,QString("CursorPlot"));
    (*it)->save(p);
  }
  QtPersistent::insert(p,QString("EndCursorX"));
}

void CursorsX::load(const char*& p)
{
  QtPWidget::load(p);

  for(std::list<CursorDefinition*>::const_iterator it=_cursors.begin(); it!=_cursors.end(); it++) {
    _names.push_back((*it)->name());
    delete *it;
  }
  _cursors.clear();

  for(std::list<CursorPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    disconnect(*it, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
    delete *it;
  }
  _plots.clear();

  QString name = QtPersistent::extract_s(p);
  while(name == QString("CursorDef")) {
    QString n = QtPersistent::extract_s(p);
    double  v = QtPersistent::extract_d(p);
    _names.removeAll(n);
    CursorDefinition* d = new CursorDefinition(n, v, *this, _frame.plot());
    _cursors.push_back(d);
    _clayout->addWidget(d);
    
    name = QtPersistent::extract_s(p);
  }

  while(name == QString("CursorPlot")) {
    CursorPlot* plot = new CursorPlot(this, p);
    _plots.push_back(plot);
    connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));

    name = QtPersistent::extract_s(p);
  }
}

void CursorsX::configure(char*& p, unsigned input, unsigned& output,
			 ChannelDefinition* channels[], int* signatures, unsigned nchannels,
			 ConfigureRequest::Source source)
{
  for(std::list<CursorPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels,_frame.xinfo(),source);
}

void CursorsX::setup_payload(Cds& cds)
{
  for(std::list<CursorPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->setup_payload(cds);
}

void CursorsX::update()
{
  for(std::list<CursorPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->update();
}

void CursorsX::set_channel(int c) 
{ 
  _channel=c; 
}

void CursorsX::add_cursor()
{
  if (_names.size()) {
    CursorDefinition* d = new CursorDefinition(_names.takeFirst(),
					       _new_value->value(),
					       *this,
					       _frame.plot());
    _cursors.push_back(d);
    _clayout->addWidget(d);
  }
  else {
    QMessageBox::critical(this,tr("Add Cursor"),tr("Too many cursors in use"));
  }
}

void CursorsX::remove(CursorDefinition& c)
{
  _names.push_back(c.name());
  _cursors.remove(&c);
  delete &c;
}

void CursorsX::calc()
{
  QStringList variables;
  for(std::list<CursorDefinition*>::const_iterator it=_cursors.begin(); it!=_cursors.end(); it++)
    variables << (*it)->name();

  QStringList ops;
  ops << _exponentiate
      << _multiply
      << _divide
      << _add   
      << _subtract;

  QStringList vops;
  vops << _integrate
       << _range;

  Calculator* c = new Calculator(tr("Cursor Math"),"",
 				 variables, vops, ops);
  if (c->exec()==QDialog::Accepted)
    _expr->setText(c->result());

   delete c;
}

void CursorsX::plot()
{
  DescEntry* desc;
  QString feature;
  switch(_plot_grp->checkedId()) {
  case _TH1F:
    desc = new Ami::DescTH1F(qPrintable(_title->text()),
			     "projection","events",
			     _hist->bins(),_hist->lo(),_hist->hi()); 
    break;
  case _vT: 
    desc = new Ami::DescScalar(qPrintable(_title->text()),
			       "projection");
    break;
  case _vF:
    desc = new Ami::DescProf(qPrintable(_title->text()),
			     qPrintable(_vFeature->expr()),"mean",
			     _vFeature->bins(),_vFeature->lo(),_vFeature->hi(),"mean");
    feature = _vFeature->feature();
    break;
  case _vS:
    desc = new Ami::DescScan(qPrintable(_title->text()),
			     qPrintable(_vScan->expr()),"mean",_vScan->bins());
    feature = _vScan->feature();
    break;
  default:
    desc = 0;
    break;
  }

  // replace cursors with values
  // and integrate symbol with 8-bit char
  QString expr = _expr->text();
  for(std::list<CursorDefinition*>::const_iterator it=_cursors.begin(); it!=_cursors.end(); it++) {
    QString new_expr;
    const QString match = (*it)->name();
    int last=0;
    int pos=0;
    while( (pos=expr.indexOf(match,pos)) != -1) {
      new_expr.append(expr.mid(last,pos-last));
      new_expr.append(QString("[%1]").arg((*it)->location()));
      pos += match.size();
      last = pos;
    }
    new_expr.append(expr.mid(last));
    expr = new_expr;
  }
  expr.replace(_integrate   ,BinMath::integrate());
  expr.replace(_range       ,BinMath::range    ());
  expr.replace(_exponentiate,Expression::exponentiate());
  expr.replace(_multiply    ,Expression::multiply());
  expr.replace(_divide      ,Expression::divide());
  expr.replace(_add         ,Expression::add());
  expr.replace(_subtract    ,Expression::subtract());

  CursorPlot* plot = new CursorPlot(this,
				    _title->text(),
				    _channel,
				    new BinMath(*desc,qPrintable(expr),qPrintable(feature)));
  _plots.push_back(plot);

  connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));

  emit changed();
}

void CursorsX::hide_cursors()
{
}

void CursorsX::remove_plot(QObject* obj)
{
  CursorPlot* plot = static_cast<CursorPlot*>(obj);
  _plots.remove(plot);

  disconnect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
}

void CursorsX::grab_cursorx() { grab_cursor(); }

void CursorsX::_set_cursor(double x, double y)
{
  _new_value->setText(QString::number(x));
  emit grabbed();
}

