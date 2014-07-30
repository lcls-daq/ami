//
//  Completes the parsing of expressions involving bin indices.
//  Indices for 1-d objects appear as [bin#] or [bin#,bin#].
//  Indices for 2-d objects appear as [bin#][bin#] or [bin#,bin#][bin#,bin#]
//  Indices for vector array objects appear as [1+element#] ([0]=size of array)
//
#include "BinMath.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScalarRange.hh"
#include "ami/data/EntryScalarDRange.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryTH2F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryProf2D.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryRef.hh"
#include "ami/data/EntryCache.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/data/VectorArray.hh"
#include "ami/data/BinMathTerms.hh"

#include "ami/data/Cds.hh"
#include "ami/data/FeatureExpression.hh"
#include "ami/data/valgnd.hh"

#include <QtCore/QChar>
#include <QtCore/QRegExp>

#include <stdio.h>
#include <sstream>

//#define DBUG

static bool _parseIndices(const QString& use, 
                          unsigned& lo, 
                          unsigned& hi,
                          Ami::Moment& mom,
                          bool lsort=true);

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
  _input     (0),
  _entry     (0),
  _v         (true),
  _index     (0),
  _loop      (false)
{
  strncpy_val(_expression, expr, EXPRESSION_LEN);
  memcpy_val (_desc_buffer, &output, output.size(),DESC_LEN);
}

#define CASETERM(type)							\
  case DescEntry::type:							\
  { t = new Ami::BinMathC::Entry##type##Term(_input,lo,hi,mom);		\
    break; }

BinMath::BinMath(const char*& p, const DescEntry& input, FeatureCache& icache, FeatureCache& ocache) :
  AbsOperator(AbsOperator::BinMath),
  _cache (&icache),
  _term  (0),
  _input (EntryFactory::entry(input)),
  _v     (true),
  _index     (0),
  _loop  (false)
{
  _extract(p, _expression , EXPRESSION_LEN);
  _extract(p, _desc_buffer, DESC_LEN);

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);

  _entry = EntryFactory::entry(o,&ocache);

  bool term_uses=false;
  _term = _process_expr(icache,_expression,term_uses);
  if (!_term) _v = false; 

  _v &= _setup(o,icache);

  delete _input;
  _input = 0;
}

BinMath::BinMath(const char*& p) :
  AbsOperator(AbsOperator::BinMath),
  _cache     (0),
  _term      (0),
  _input     (0),
  _entry     (0),
  _v         (true),
  _index     (0),
  _loop      (false)
{
  _extract(p, _expression , EXPRESSION_LEN);
  _extract(p, _desc_buffer, DESC_LEN);
}

BinMath::~BinMath()
{
  if (_term ) delete _term;
  if (_entry) delete _entry;
}

void BinMath::use() {
  _use();
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
  if (!e.valid())
    return *_entry;

  if (_term) {
    _input = &e;

#ifdef DBUG
    printf("BinMath::_operate _term %f\n",y);
#endif

    unsigned n = (e.desc().type()==DescEntry::Ref && _loop) ?
      reinterpret_cast<const VectorArray*>
      (static_cast<const EntryRef&>(e).data())->nentries() : 1;

    for(_index=0; _index<n; _index++) {
      double y = _term->evaluate();
      _fill(*_entry, y, e.time(), _index==0);
    }
  }
  return *_entry;
}

static bool _parseIndices(const QString& use, 
                          unsigned& lo, 
                          unsigned& hi,
                          Moment&   mom,
                          bool      lsort)
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

  if (hi < lo && lsort) {
    unsigned t = lo;
    lo = hi;
    hi = t;
  }

  return true;
}

Term* BinMath::_process_expr(FeatureCache& input,
                             const char* iexpr,
                             bool& term_uses)
{
  QString expr(iexpr);
  DescEntry::Type type = _input->desc().type();
  QString new_expr;
  // parse expression for bin indices
  QString mex("\\[[0-9");
  mex += _integrate;
  mex += _mean;
  mex += _variance;
  mex += _contrast;
  mex += _xmoment;
  mex += _ymoment;
  //    mex += QString("\\\%1").arg(_xmoment);
  //    mex += QString("\\\%1").arg(_ymoment);
  mex += QString("\\\'\\\"]+\\]");
  QRegExp match(mex);
  int last=0;
  int pos=0;
  int mlen=0;
  std::list<Term*> free;
  while( (pos=match.indexIn(expr,pos)) != -1) {
    mlen = match.matchedLength();
    QString use = expr.mid(pos+1,mlen-2);
    unsigned lo, hi;
    Moment mom;
    if (!_parseIndices(use,lo,hi,mom)) {
      pos += mlen;
      continue;
    }
    Term* t = 0;
    switch(type) {
      CASETERM(Waveform);
      CASETERM(TH1F);
      CASETERM(Prof);
    case DescEntry::Ref:  // VectorArray
      if (lo==0)
	t = new Ami::BinMathC::VASizeTerm(_input);
      else {
	t = new Ami::BinMathC::VAElementTerm(_input,_index,lo-1);
	_loop = true;
      }
      break;
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
      printf("BinMath: No implementation for entry type %d\n",type);
      t = 0;
      break;
    }
    new_expr.append(expr.mid(last,pos-last));
    new_expr.append(QString("[%1]").arg((ulong)t,0,16));
    pos += mlen;
    last = pos;
    if (t) free.push_back(t);
  }
  new_expr.append(expr.mid(last));

  term_uses = (new_expr!=iexpr);
  FeatureExpression parser;
  Term* term = parser.evaluate(input,new_expr);
  if (!term) {
    printf("BinMath failed to parse %s [%s]\n",
           qPrintable(new_expr),iexpr);
    for(std::list<Term*>::iterator it=free.begin();
        it!=free.end(); it++)
      delete *it;
  }
  return term;
}

void BinMath::_invalid() {}

std::string BinMath::_text() const
{
  std::ostringstream s;
  s << _expression << ":" 
    << DescEntry::type_str(_entry->desc().type()) << "["
    << _entry->desc().name() << "]";
  return s.str();
}
