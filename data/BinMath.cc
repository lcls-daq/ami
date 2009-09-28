#include "BinMath.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/data/Cds.hh"
#include "ami/data/Expression.hh"

#include <QtCore/QString>
#include <QtCore/QChar>
#include <QtCore/QRegExp>

#include <stdio.h>

//
//  this could probably be a template
//
#define CLASSTERM(type,func) \
  class Entry##type##Term : public Ami::Term {				\
  public:								\
    Entry##type##Term(const Entry*& e, unsigned lo, unsigned hi) :	\
      _entry(e), _lo(lo), _hi(hi) {}					\
      ~Entry##type##Term() {}						\
  public:								\
      double evaluate() const						\
      { double sum=0;							\
	for(unsigned i=_lo; i<=_hi; i++)				\
	  sum += static_cast<const Entry##type*>(_entry)->func(i);	\
	return sum; }							\
  private:								\
      const Entry*& _entry;						\
      unsigned _lo, _hi;						\
  }

namespace Ami {
  namespace BinMathC {
    CLASSTERM(Waveform,content);
    CLASSTERM(TH1F    ,content);
    CLASSTERM(Prof    ,ymean  );
    CLASSTERM(Image   ,content); // This isn't right for summing over 2-dimensions
  };
};

using namespace Ami;

static QChar _integrate(0x002C);
static QChar _range    (0x0023);
const QChar& BinMath::integrate() { return _integrate; }
const QChar& BinMath::range    () { return _range    ; }

BinMath::BinMath(const DescEntry& output, 
		 const char* expr,
		 unsigned feature_index) :
  AbsOperator(AbsOperator::BinMath),
  _feature_index(feature_index),
  _cache     (0),
  _term      (0),
  _entry     (0)
{
  strncpy(_expression, expr, EXPRESSION_LEN);
  memcpy (_desc_buffer, &output, output.size());
}

#define CASETERM(type)							\
  case DescEntry::type:							\
  { t = new Ami::BinMathC::Entry##type##Term(_input,lo,hi);		\
    break; }

BinMath::BinMath(const char*& p, const DescEntry& input, FeatureCache& features) :
  AbsOperator(AbsOperator::BinMath),
  _cache (&features)
{
  _extract(p, _expression , EXPRESSION_LEN);
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_feature_index, sizeof(_feature_index));

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);

  _entry = EntryFactory::entry(o);
 
  QString expr(_expression);
  QString new_expr;

  {  // parse expression for bin indices
    QRegExp match("\\[[0-9,]+\\]");
    int last=0;
    int pos=0;
    while( (pos=match.indexIn(expr,pos)) != -1) {
      QString use = expr.mid(pos+1,match.matchedLength()-2);
      unsigned lo, hi;
      int index = use.indexOf(_integrate);
      if (index == -1)
	lo = hi = use.toInt();
      else {
	lo = use.mid(0,index).toInt();
	hi = use.mid(index+1,-1).toInt();
	if (lo > hi) { unsigned i=lo; lo=hi; hi=i; }
      }
      Term* t;
      switch(input.type()) {
	CASETERM(Waveform);
	CASETERM(TH1F);
	CASETERM(Prof);
	CASETERM(Image);
      default:
	printf("BinMath: No implementation for entry type %d\n",input.type());
	t = 0;
	break;
      }
      new_expr.append(expr.mid(last,pos-last));
      new_expr.append(QString("[%1]").arg((ulong)t,0,16));
      pos += match.matchedLength();
      last = pos;
    }
    new_expr.append(expr.mid(last));
  }

  std::list<Variable*> variables; // none
  Expression parser(variables);
  _term = parser.evaluate(new_expr);
  if (!_term)
    printf("BinMath failed to parse %s\n",qPrintable(new_expr));
}

BinMath::~BinMath()
{
  if (_term) delete _term;
  if (_entry) delete _entry;
}

DescEntry& BinMath::output   () const 
{ 
  return _entry ? _entry->desc() : *reinterpret_cast<DescEntry*>(const_cast<char*>(_desc_buffer)); 
}
unsigned BinMath::feature_index() const { return _feature_index; }

const char* BinMath::expression() const { return _expression; }

void*      BinMath::_serialize(void* p) const
{
  _insert(p, _expression, EXPRESSION_LEN);
  _insert(p, _desc_buffer, DESC_LEN);
  _insert(p, &_feature_index, sizeof(_feature_index));
  return p;
}

Entry&     BinMath::_operate(const Entry& e) const
{
  if (_term) {
    _input = &e;
    double y = _term->evaluate();
    switch(_entry->desc().type()) {
    case DescEntry::Scalar:  static_cast<EntryScalar*>(_entry)->addcontent(y);    break;
    case DescEntry::TH1F:    static_cast<EntryTH1F*  >(_entry)->addcontent(1.,y); break;
    case DescEntry::Prof:    
      { bool damaged; double x=_cache->cache(_feature_index,&damaged);
	if (!damaged)
	  static_cast<EntryProf*  >(_entry)->addy(y,x);
	break; }
    case DescEntry::Waveform:
    case DescEntry::TH2F:
    case DescEntry::Image:
    default:
      printf("BinMath::_operator no implementation for type %d\n",_entry->desc().type());
      break;
    }
  }
  _entry->time(e.time());
  return *_entry;
}
