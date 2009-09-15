#include "CursorsX.hh"

#include "ami/qt/DescTH1F.hh"
#include "ami/qt/DescProf.hh"
#include "ami/qt/DescChart.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/CursorDefinition.hh"
#include "ami/qt/CursorPlot.hh"
#include "ami/qt/Display.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/Calculator.hh"
#include "ami/qt/PlotFrame.hh"

#include "ami/data/DescScalar.hh"
#include "ami/data/DescTH1F.hh"
#include "ami/data/DescProf.hh"
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

enum { _TH1F, _vT, _vF };


namespace Ami {
  namespace Qt {
#if 0
    class CursorLocation : public QDoubleSpinBox {
    public:
      CursorLocation(double* x, unsigned nx, int b=0) : _x(x), _nx(nx), _b(b) {}
      ~CursorLocation() {}
    public:
      int location  () const { return _b; }
      int location  (double v) const 
      {
	for(unsigned k=0; k<_nx && _xx[k]<v; k++) ;
	return k ? k-1 : k;
      }
      double   location  (int v) const { return _x[v]; }
      void     stepBy    (int steps) 
      {
	_b += steps;
	if (_b<0)    _b = 0;
	if (_b>=_nx) _b = _nx-1;
      }
    public:
      QString textFromValue(double         v) const { return QString::number(location(_b=location(v))); }
      double  valueFromText(const QString& t) const { return location(location(t.toDouble())); }
    private:
      int _b;
      double   _lo, _hi;
    };
#else
    class CursorLocation : public QLineEdit {
    public:
      CursorLocation() : QLineEdit("0") { new QDoubleValidator(this); }
      ~CursorLocation() {}
    public:
      double value() const { return text().toDouble(); }
    };
#endif
  };
};

using namespace Ami::Qt;

static QChar _integrate   (0x2026);
static QChar _exponentiate(0x005E);
static QChar _multiply    (0x00D7);
static QChar _divide      (0x00F7);
static QChar _add         (0x002B);
static QChar _subtract    (0x002D);

CursorsX::CursorsX(ChannelDefinition* channels[], unsigned nchannels, Display& frame) :
  QWidget   (0),
  Cursors   (*frame.plot()),
  _channels (channels),
  _nchannels(nchannels),
  _channel  (0),
  _frame    (frame),
  _clayout  (new QVBoxLayout),
  _expr     (new QLineEdit("1")),
  _title    (new QLineEdit("Cursor plot")),
  _features (new QComboBox)
{
  _names << "a" << "b" << "c" << "d" << "e" << "f" << "g";

  _expr->setReadOnly(true);

  _new_value = new CursorLocation;

  setWindowTitle("CursorsX Plot");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QComboBox* channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    channelBox->addItem(channels[i]->name());

  _features->addItems(FeatureRegistry::instance().names());

  _hist   = new DescTH1F (bold(Sum (1dH)));
  _vTime  = new DescChart(bold(Sum v Time),0.2);
  _vFeature = new DescProf (bold(Sum v Var),_features);

  _plot_grp = new QButtonGroup;
  _plot_grp->addButton(_hist    ->button(),_TH1F);
  _plot_grp->addButton(_vTime   ->button(),_vT);
  _plot_grp->addButton(_vFeature->button(),_vF);
  _hist->button()->setChecked(true);

  QPushButton* calcB  = new QPushButton("Enter");
  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* closeB = new QPushButton("Close");
  QPushButton* loadB  = new QPushButton("Load");
  QPushButton* saveB  = new QPushButton("Save");
  QPushButton* grabB  = new QPushButton("Grab");
  
  QVBoxLayout* layout = new QVBoxLayout;
  { QGroupBox* file_box = new QGroupBox("File");
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(loadB);
    layout1->addWidget(saveB);
    layout1->addStretch();
    file_box->setLayout(layout1);
    layout->addWidget(file_box); }
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
				 "  A %2 B : Exponentiate [value at] A to the power of [value at] B\n "	\
				 "  A %3 B : Multiply [value at] A by [value at] B\n "	\
				 "  A %4 B : Divide \n "	\
				 "  A %5 B : Add \n "		\
				 "  A %6 B : Subtract \n ")
			 .arg(_integrate)
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
  connect(loadB     , SIGNAL(clicked()),      this, SLOT(load()));
  connect(saveB     , SIGNAL(clicked()),      this, SLOT(save()));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));
  connect(&FeatureRegistry::instance(), SIGNAL(changed()), this, SLOT(change_features()));
  connect(grabB     , SIGNAL(clicked()),      this, SLOT(grab_cursorx()));
  connect(this      , SIGNAL(grabbed()),      this, SLOT(add_cursor()));
}
  
CursorsX::~CursorsX()
{
}

void CursorsX::configure(char*& p, unsigned input, unsigned& output,
			ChannelDefinition* channels[], int* signatures, unsigned nchannels)
{
  for(std::list<CursorPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels,_frame.xinfo());
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
  vops << _integrate;

  Calculator* c = new Calculator(tr("Cursor Math"),"",
 				 variables, vops, ops);
  if (c->exec()==QDialog::Accepted)
    _expr->setText(c->result());

   delete c;
}

void CursorsX::load()
{
}

void CursorsX::save()
{
}

void CursorsX::plot()
{
  DescEntry* desc;
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
			     qPrintable(_vFeature->variable()),"mean",
			     _vFeature->bins(),_vFeature->lo(),_vFeature->hi(),"mean");
    break;
  default:
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
  expr.replace(_exponentiate,Expression::exponentiate());
  expr.replace(_multiply    ,Expression::multiply());
  expr.replace(_divide      ,Expression::divide());
  expr.replace(_add         ,Expression::add());
  expr.replace(_subtract    ,Expression::subtract());

  CursorPlot* plot = new CursorPlot(_title->text(),
				    new BinMath(_channel,*desc,qPrintable(expr),
						FeatureRegistry::instance().index(_vFeature->variable())));
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

void CursorsX::change_features()
{
  _features->clear();
  _features->addItems(FeatureRegistry::instance().names());
  _features->setCurrentIndex(0);
}
