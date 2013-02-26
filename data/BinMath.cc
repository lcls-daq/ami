//
//  Completes the parsing of expressions involving bin indices.
//  Indices for 1-d objects appear as [bin#] or [bin#,bin#].
//  Indices for 2-d objects appear as [bin#][bin#] or [bin#,bin#][bin#,bin#]
//
#include "BinMath.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScalarRange.hh"
#include "ami/data/EntryScalarDRange.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryTH2F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryCache.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/data/BinMathTerms.hh"

#include "ami/data/Cds.hh"
#include "ami/data/FeatureExpression.hh"

#include <QtCore/QString>
#include <QtCore/QChar>
#include <QtCore/QRegExp>

#include <stdio.h>

static bool _parseIndices(const QString& use, 
                          unsigned& lo, 
                          unsigned& hi,
                          Ami::Moment& mom);

static const unsigned MAX_INDEX = 999999;

using namespace Ami;

static QChar _integrate(0x002C);  // ,
static QChar _mean     (0x003D);  // @
static QChar _variance (0x005F);  // _
static QChar _moment1  (0x0027);  // '
static QChar _moment2  (0x0022);  // "
static QChar _contrast (0x0021);  // !
static QChar _range    (0x0023);  // #
static QChar _xmoment  (0x0024);  // $
static QChar _ymoment  (0x003F);  // &
const QChar& BinMath::integrate() { return _integrate; }
const QChar& BinMath::mean     () { return _mean     ; }
const QChar& BinMath::variance () { return _variance ; }
const QChar& BinMath::moment1  () { return _moment1  ; }
const QChar& BinMath::moment2  () { return _moment2  ; }
const QChar& BinMath::contrast () { return _contrast ; }
const QChar& BinMath::range    () { return _range    ; }
const QChar& BinMath::xmoment  () { return _xmoment  ; }
const QChar& BinMath::ymoment  () { return _ymoment  ; }
const double BinMath::floatPrecision() { return 1.e3; }

BinMath::BinMath(const DescEntry& output, 
		 const char* expr) :
  AbsOperator(AbsOperator::BinMath),
  _cache     (0),
  _term      (0),
  _fterm     (0),
  _entry     (0),
  _v         (true)
{
  strncpy(_expression, expr, EXPRESSION_LEN);
  memcpy (_desc_buffer, &output, output.size());
  memset (_desc_buffer+output.size(), 0, DESC_LEN-output.size());
}

#define CASETERM(type)							\
  case DescEntry::type:							\
  { t = new Ami::BinMathC::Entry##type##Term(_input,lo,hi,mom);		\
    break; }

BinMath::BinMath(const char*& p, const DescEntry& input, FeatureCache& features) :
  AbsOperator(AbsOperator::BinMath),
  _cache (&features),
  _term  (0),
  _fterm (0),
  _v     (true)
{
  _extract(p, _expression , EXPRESSION_LEN);
  _extract(p, _desc_buffer, DESC_LEN);

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);

  _entry = EntryFactory::entry(o);
 
  { QString expr(_expression);
    QString new_expr;
    // parse expression for bin indices
    QString mex("\\[[0-9");
    mex += _integrate;
    mex += _mean;
    mex += _variance;
    mex += _contrast;
    mex += _xmoment;
    mex += _ymoment;
    mex += QString("\\\'\\\"]+\\]");
    QRegExp match(mex);
    int last=0;
    int pos=0;
    int mlen=0;
    while( (pos=match.indexIn(expr,pos)) != -1) {
      mlen = match.matchedLength();
      QString use = expr.mid(pos+1,mlen-2);
      unsigned lo, hi;
      Moment mom;
      if (!_parseIndices(use,lo,hi,mom)) {
        pos += mlen;
        continue;
      }
      Term* t;
      switch(input.type()) {
	CASETERM(Waveform);
	CASETERM(TH1F);
	CASETERM(Prof);
      case DescEntry::Image:
	{ int ypos = pos+mlen;
	  ypos = match.indexIn(expr,ypos);
	  mlen += match.matchedLength();
	  QString yuse = expr.mid(ypos+1,match.matchedLength()-2);
	  unsigned ylo, yhi;  _parseIndices(yuse,ylo,yhi,mom);
	  if (expr[pos+mlen]=='[') {  // third dimension (azimuth)
	    int zpos = pos+mlen;
	    zpos = match.indexIn(expr,zpos);
	    mlen += match.matchedLength();
	    QString zuse = expr.mid(zpos+1,match.matchedLength()-2);
	    unsigned zlo, zhi;  _parseIndices(zuse,zlo,zhi,mom);
	    t = new Ami::BinMathC::EntryImageTermF(_input,
                                                   double( lo)/floatPrecision(),double( hi)/floatPrecision(),
                                                   double(ylo)/floatPrecision(),double(yhi)/floatPrecision(),
                                                   double(zlo)/floatPrecision(),double(zhi)/floatPrecision(),
                                                   mom); 
	  }
	  else
	    t = new Ami::BinMathC::EntryImageTerm(_input,lo,hi,ylo,yhi,mom); 
	}
	break;
      default:
	printf("BinMath: No implementation for entry type %d\n",input.type());
	t = 0;
	break;
      }
      new_expr.append(expr.mid(last,pos-last));
      new_expr.append(QString("[%1]").arg((ulong)t,0,16));
      pos += mlen;
      last = pos;
    }
    new_expr.append(expr.mid(last));

//     std::list<Variable*> variables; // none
//     Expression parser(variables);
//     _term = parser.evaluate(new_expr);
    FeatureExpression parser;
    _term = parser.evaluate(features,new_expr);
    if (!_term) {
      printf("BinMath failed to parse %s (%s)\n",qPrintable(new_expr),_expression);
      _v = false; 
    }
  }

  if (o.type() == DescEntry::Prof ||
      o.type() == DescEntry::Scan ||
      o.type() == DescEntry::TH2F) {
    QString expr(o.xtitle());
    FeatureExpression parser;
    _fterm = parser.evaluate(features,expr);
    if (!_fterm) {
      printf("BinMath failed to parse f %s\n",qPrintable(expr));
      _v = false;
    }
  }
}

BinMath::BinMath(const char*& p) :
  AbsOperator(AbsOperator::BinMath),
  _cache     (0),
  _term      (0),
  _fterm     (0),
  _entry     (0),
  _v         (true)
{
  _extract(p, _expression , EXPRESSION_LEN);
  _extract(p, _desc_buffer, DESC_LEN);
}

BinMath::~BinMath()
{
  if (_term) delete _term;
  if (_fterm) delete _fterm;
  if (_entry) delete _entry;
}

DescEntry& BinMath::_routput   () const 
{ 
  return _entry ? _entry->desc() : *reinterpret_cast<DescEntry*>(const_cast<char*>(_desc_buffer)); 
}

const char* BinMath::expression() const { return _expression; }

void*      BinMath::_serialize(void* p) const
{
  _insert(p, _expression , EXPRESSION_LEN);
  _insert(p, _desc_buffer, DESC_LEN);
  return p;
}

Entry&     BinMath::_operate(const Entry& e) const
{
  _entry->valid(e.time());
  if (!e.valid())
    return *_entry;

  if (_term) {
    _input = &e;
    double y = _term->evaluate();

    switch(_entry->desc().type()) {
    case DescEntry::Scalar:  
      { EntryScalar* en = static_cast<EntryScalar*>(_entry);
	en->addcontent(y);
	break; }
    case DescEntry::ScalarRange:  
      { EntryScalarRange* en = static_cast<EntryScalarRange*>(_entry);
	en->addcontent(y);
	break; }
    case DescEntry::ScalarDRange:  
      if (!_fterm)
	return *_entry;
      { bool damaged=false; double x=_fterm->evaluate();
	if (!damaged) {
          EntryScalarDRange* en = static_cast<EntryScalarDRange*>(_entry);
          en->addcontent(x,y);
        }
	break; }
    case DescEntry::TH1F:
      { EntryTH1F* en = static_cast<EntryTH1F*  >(_entry);
	en->addcontent(1.,y); 
	en->addinfo(1.,EntryTH1F::Normalization);
	break; }
    case DescEntry::TH2F:
      if (!_fterm)
	return *_entry;
      { bool damaged=false; double x=_fterm->evaluate();
	if (!damaged) {
          EntryTH2F* en = static_cast<EntryTH2F*  >(_entry);
          en->addcontent(1.,x,y); 
          en->addinfo(1.,EntryTH2F::Normalization);
        }
	break; }
    case DescEntry::Prof:  
      if (!_fterm)
	return *_entry;
     { bool damaged=false; double x=_fterm->evaluate();
	if (!damaged) {
	  EntryProf* en = static_cast<EntryProf*  >(_entry);
	  en->addy(y,x);
	  en->addinfo(1.,EntryProf::Normalization);
	}
	break; }
    case DescEntry::Scan:    
      if (!_fterm)
	return *_entry;
      { bool damaged=false; double x=_fterm->evaluate();
	if (!damaged) {
	  EntryScan* en = static_cast<EntryScan*  >(_entry);
	  en->addy(y,x,1.,e.last());
	  en->addinfo(1.,EntryScan::Normalization);
	}
	break; }
    case DescEntry::Cache:
      { EntryCache* en = static_cast<EntryCache*>(_entry);
        en->set(y,false);
        break; }
    case DescEntry::Waveform:
    case DescEntry::Image:
    default:
      printf("BinMath::_operator no implementation for type %d\n",_entry->desc().type());
      break;
    }
  }
  return *_entry;
}

static bool _parseIndices(const QString& use, 
                          unsigned& lo, 
                          unsigned& hi,
                          Moment&   mom)
{
  int index;
  if ((index=use.indexOf(_integrate)) == -1)
    if ((index=use.indexOf(_mean)) == -1)
      if ((index=use.indexOf(_variance)) == -1)
        if ((index=use.indexOf(_moment1)) == -1)
          if ((index=use.indexOf(_moment2)) == -1) 
            if ((index=use.indexOf(_contrast)) == -1)
              if ((index=use.indexOf(_xmoment)) == -1)
                if ((index=use.indexOf(_ymoment)) == -1) {
                  mom = None;
                  lo = hi = use.toInt();
                  return lo <  MAX_INDEX;
                }
                else
                  mom = YCenterOfMass;
              else
                mom = XCenterOfMass;
            else
              mom = Contrast;
          else
            mom = Second;
        else 
          mom = First;
      else 
        mom = Variance;
    else 
      mom = Mean;
  else
    mom = Zero;

  lo = use.mid(0,index).toInt();
  hi = use.mid(index+1,-1).toInt();

  return true;
}

