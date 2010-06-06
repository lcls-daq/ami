#include "ami/qt/ChannelMath.hh"

#include "ami/qt/Calculator.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Filter.hh"

#include "ami/data/LogicAnd.hh"
#include "ami/data/AbsFilter.hh"
#include "ami/data/AbsOperator.hh"
#include "ami/data/EntryMath.hh"
#include "ami/data/Expression.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

using namespace Ami::Qt;

ChannelMath::ChannelMath(const QStringList& names) :
  QWidget (0),
  _expr   (new QLineEdit),
  _changed(false),
  _names  (names),
  _filter (0),
  _operator(0)
{
  _expr->setReadOnly(true);
  QPushButton* calcB = new QPushButton("Enter");

  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(new QLabel("Expr"));
  layout->addWidget(_expr);
  layout->addWidget(calcB);
  setLayout(layout);

  connect(calcB, SIGNAL(clicked()), this, SLOT(calc()));
}

ChannelMath::~ChannelMath() 
{
  if (_filter  ) delete _filter;
  if (_operator) delete _operator;
}

static QChar _exponentiate(0x005E);
static QChar _multiply    (0x00D7);
static QChar _divide      (0x00F7);
static QChar _add         (0x002B);
static QChar _subtract    (0x002D);

void ChannelMath::calc()
{
  QStringList ops;
  ops << _exponentiate
      << _multiply
      << _divide
      << _add   
      << _subtract;

  QStringList vops;

  Calculator* c = new Calculator(tr("Channel Math"),"",
				 _names, vops, ops);
  if (c->exec()==QDialog::Accepted) {
    _expr->setText(c->result());
    _changed = true;
  }

  delete c;
}

bool ChannelMath::resolve(ChannelDefinition* channels[],
			  int*  signatures,
			  int nchannels,
			  const Ami::AbsFilter& f)
{
  QStringList names;
  for(int i=0; i<nchannels; i++)
    names << channels[i]->name();

  QString expr(_expr->text());
  expr.replace(_exponentiate,Expression::exponentiate());
  expr.replace(_multiply    ,Expression::multiply());
  expr.replace(_divide      ,Expression::divide());
  expr.replace(_add         ,Expression::add());
  expr.replace(_subtract    ,Expression::subtract());

  QStringList uses;
  QRegExp match("[a-zA-Z_]+[0-9]*");
  int pos=0;
  while( (pos=match.indexIn(expr,pos)) != -1) {
    QString use = expr.mid(pos,match.matchedLength());
    if (!uses.contains(use))
      uses << use;
    pos += match.matchedLength();
  }

  _signatures.clear();
  for(QStringList::iterator it=uses.begin(); it!=uses.end(); it++) {
    int index=names.indexOf(*it);
    if (index<0) {
      printf("CM::resolve index not found\n");
      return false;
    }
    if (signatures[index]<0) {
      printf("CM::resolve signature[%d] not found\n",index);
      return false;
    }
    _signatures << signatures[index];
  }

  if (_filter) delete _filter;
  _filter = f.clone();
  for(int i=0; i<nchannels; i++) {
    if (uses.contains(QString(channels[i]->name())))
      _filter = new LogicAnd(*_filter, *channels[i]->filter().filter()->clone());
  }

  // replace all channel names with their signatures
  for(int i=0; i<nchannels; i++)
    expr.replace(channels[i]->name(),QString("[%1]").arg(signatures[i]));

  if (_operator) delete _operator;
  _operator = new EntryMath(qPrintable(expr));

  return true;
}

const Ami::AbsFilter& ChannelMath::filter() const
{
  return *_filter;
}

const Ami::AbsOperator& ChannelMath::op () const
{
  return *_operator;
}

QString   ChannelMath::expr() const
{
  return _expr->text();
}

void      ChannelMath::expr(const QString& t)
{
  _expr->setText(t);
}
