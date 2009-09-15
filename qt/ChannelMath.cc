#include "ami/qt/ChannelMath.hh"

#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Filter.hh"

#include "ami/data/LogicAnd.hh"
#include "ami/data/AbsFilter.hh"
#include "ami/data/AbsOperator.hh"
#include "ami/data/EntryMath.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

using namespace Ami::Qt;

ChannelMath::ChannelMath() :
  QWidget (0),
  _expr   (new QLineEdit),
  _changed(false)
{
  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(new QLabel("Expr"));
  layout->addWidget(_expr);
  setLayout(layout);

  connect(_expr   , SIGNAL(editingFinished()), this, SLOT(change_expr()));
}

ChannelMath::~ChannelMath() 
{
}

void ChannelMath::change_expr()
{
  _changed = true;
}

bool ChannelMath::resolve(ChannelDefinition* channels[],
			  int*  signatures,
			  int nchannels,
			  const Ami::AbsFilter& f)
{
  QStringList names;
  for(int i=0; i<nchannels; i++)
    names << channels[i]->name();

  QStringList uses;
  QRegExp match("[a-zA-Z]+");
  int pos=0;
  QString expr(_expr->text());
  while( (pos=match.indexIn(expr,pos)) != -1) {
    QString use = expr.mid(pos,match.matchedLength());
    if (!uses.contains(use))
      uses << use;
    pos += match.matchedLength();
  }

  _signatures.clear();
  for(QStringList::iterator it=uses.begin(); it!=uses.end(); it++) {
    int index=names.indexOf(*it);
    if (index<0)
      return false;
    if (signatures[index]<0)
      return false;
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

