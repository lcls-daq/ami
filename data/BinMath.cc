//
//  Completes the parsing of expressions involving bin indices.
//  Indices for 1-d objects appear as [bin#] or [bin#,bin#].
//  Indices for 2-d objects appear as [bin#][bin#] or [bin#,bin#][bin#,bin#]
//
#include "BinMath.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/DescWaveform.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScalarRange.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryCache.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/ImageMask.hh"

#include "ami/data/Cds.hh"
#include "ami/data/FeatureExpression.hh"

#include <QtCore/QString>
#include <QtCore/QChar>
#include <QtCore/QRegExp>

#include <stdio.h>

enum Moment { None, Zero, First, Second, Contrast };

static bool _parseIndices(const QString& use, 
                          unsigned& lo, 
                          unsigned& hi,
                          Moment&   mom);

static const unsigned MAX_INDEX = 999999;

namespace Ami {
  namespace BinMathC {
    class EntryWaveformTerm : public Ami::Term {
    public:
      EntryWaveformTerm(const Entry*& e, unsigned lo, unsigned hi,
                        Moment mom) :
        _entry(e), _lo(lo), _hi(hi), _mom(mom) {}
      ~EntryWaveformTerm() {}
    public:
      double evaluate() const
      { double sum=0;
	unsigned lo=_lo, hi=_hi;
	const EntryWaveform* e = static_cast<const EntryWaveform*>(_entry);
        unsigned offset = 0;
        const DescWaveform& desc = e->desc();
        /*
         * xlow is 0.0 for all regular waveforms.  However, for references read from a .dat file,
         * the X values are the *middle* of the bins, and do not start at 0.0!  (Because of this,
         * we use floor below instead of round in calculating the offset.)
         */
        if (desc.xlow() != 0.0) {
          offset = (int) floor(desc.xlow() * (desc.nbins() - 1) / (desc.xup() - desc.xlow()));
          lo -= offset;
          hi -= offset;
        }
        switch(_mom) {
        case First:
          {
            double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
            double x  = desc.xlow()+(double(lo)+0.5)*dx;
            for(unsigned i=lo; i<=hi; i++, x+=dx)
              sum += e->content(i)*x;
          }
          break;
        case Second:
          {
            double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
            double x  = desc.xlow()+(double(lo)+0.5)*dx;
            for(unsigned i=lo; i<=hi; i++, x+=dx)
              sum += e->content(i)*x*x;
          }
          break;
        default:
          for(unsigned i=lo; i<=hi; i++)
            sum += e->content(i);
          break;
        }
	double n = e->info(EntryWaveform::Normalization);
	return n > 0 ? sum / n : sum; }
    private:
      const Entry*& _entry;
      unsigned _lo, _hi;
      Moment _mom;
    };

    class EntryTH1FTerm : public Ami::Term {
    public:
      EntryTH1FTerm(const Entry*& e, unsigned lo, unsigned hi,
                    Moment mom) :
        _entry(e), _lo(lo), _hi(hi), _mom(mom) {}
      ~EntryTH1FTerm() {}
    public:
      double evaluate() const
      { double sum=0;
        unsigned lo=_lo, hi=_hi;
        const EntryTH1F* e = static_cast<const EntryTH1F*>(_entry);
        const DescTH1F& desc = e->desc();
        switch(_mom) {
        case First:
          {
            double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
            double x  = desc.xlow()+(double(lo)+0.5)*dx;
            for(unsigned i=lo; i<=hi; i++, x+=dx)
              sum += e->content(i)*x;
          }
          break;
        case Second:
          {
            double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
            double x  = desc.xlow()+(double(lo)+0.5)*dx;
            for(unsigned i=lo; i<=hi; i++, x+=dx)
              sum += e->content(i)*x*x;
          }
          break;
        default:
          for(unsigned i=lo; i<=hi; i++)
            sum += e->content(i);
          break;
        }
        double n = e->info(EntryTH1F::Normalization);
        return n > 0 ? sum / n : sum; }
    private:
      const Entry*& _entry;
      unsigned _lo, _hi;
      Moment _mom;
    };

    class EntryProfTerm  : public Ami::Term {
    public:
      EntryProfTerm(const Entry*& e, unsigned lo, unsigned hi,
                    Moment mom) :
        _entry(e), _lo(lo), _hi(hi), _mom(mom) {}
      ~EntryProfTerm() {}
    public:
      double evaluate() const
      { double sum=0;
        unsigned lo=_lo, hi=_hi;
        const EntryProf* e = static_cast<const EntryProf*>(_entry);
        const DescProf& desc = e->desc();
        switch(_mom) {
        case First:
          {
            double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
            double x  = desc.xlow()+(double(lo)+0.5)*dx;
            for(unsigned i=lo; i<=hi; i++, x+=dx)
              sum += e->ymean(i)*x;
          }
          break;
        case Second:
          {
            double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
            double x  = desc.xlow()+(double(lo)+0.5)*dx;
            for(unsigned i=lo; i<=hi; i++, x+=dx)
              sum += e->ymean(i)*x*x;
          }
          break;
        default:
          for(unsigned i=lo; i<=hi; i++)
            sum += e->ymean(i);
          break;
        }
        return sum; }
    private:
      const Entry*& _entry;
      unsigned _lo, _hi;
      Moment _mom;
    };
    
    class EntryImageTerm : public Ami::Term {
    public:
      EntryImageTerm(const Entry*& e, 
                     unsigned xlo, unsigned xhi, 
                     unsigned ylo, unsigned yhi,
                     Moment mom) :
	_entry(e), _xlo(xlo), _xhi(xhi), _ylo(ylo), _yhi(yhi), _mom(mom) 
      {
        switch(_mom) {
        case First: 
          printf("EntryImageTerm first moment not implemented\n"); 
          break;
        case Second: 
          printf("EntryImageTerm second moment not implemented\n"); 
          break;
        default:
          break;
        }
      }
      ~EntryImageTerm() {}
    public:
      double evaluate() const {
	const EntryImage& e = *static_cast<const EntryImage*>(_entry);
	const DescImage&  d = e.desc();
        const ImageMask* mask = d.mask();
	double s0 = 0, sum = 0, sqsum = 0;
	double p   = double(e.info(EntryImage::Pedestal));
        if (mask) {
	  unsigned xlo=_xlo, xhi=_xhi, ylo=_ylo, yhi=_yhi;
          for(unsigned j=ylo; j<=yhi; j++) {
            if (!mask->row(j)) continue;
            for(unsigned i=xlo; i<=xhi; i++)
              if (mask->rowcol(j,i)) {
                double v = double(e.content(i,j))-p;
                s0    += 1;
                sum   += v;
                sqsum += v*v;
              }
          }
        }
	else if (d.nframes()) {
	  for(unsigned fn=0; fn<d.nframes(); fn++) {
	    int xlo(_xlo), xhi(_xhi+1), ylo(_ylo), yhi(_yhi+1);
	    if (d.xy_bounds(xlo, xhi, ylo, yhi, fn)) {
              for(int j=ylo; j<yhi; j++)
                for(int i=xlo; i<xhi; i++) {
                  double v = double(e.content(i,j))-p;
                  s0    += 1;
                  sum   += v;
                  sqsum += v*v;
                }
            }
	  }
	}
	else {
	  unsigned xlo=_xlo, xhi=_xhi, ylo=_ylo, yhi=_yhi;
          for(unsigned j=ylo; j<=yhi; j++)
            for(unsigned i=xlo; i<=xhi; i++) {
              double v = double(e.content(i,j))-p;
              s0    += 1;
              sum   += v;
              sqsum += v*v;
            }
	}
	double n = double(e.info(EntryImage::Normalization));
        double v;
        switch(_mom) {
        case Zero:
          v = sum;
          if (n>0) v/=n;
          break;
        case Contrast:
          v = sqrt(s0*sqsum/(sum*sum) - 1);
          //          if (n>0) v/=sqrt(n);
          break;
        default:
          v = 0;
          break;
        }
        return v;
      }
    private:
      const Entry*& _entry;
      unsigned _xlo, _xhi, _ylo, _yhi;
      Moment _mom;
    };

    class EntryImageTermF : public Ami::Term {
    public:
      EntryImageTermF(const Entry*& e, 
                      double xc, double yc, 
                      double r0, double r1, 
                      double f0, double f1,
                      Moment mom) :
	_entry(e), _xc(xc), _yc(yc), _r0(r0), _r1(r1), _f0(f0), _f1(f1), _mom(mom) 
      {
        switch(_mom) {
        case First: 
          printf("EntryImageTermF first moment not implemented\n"); 
          break;
        case Second: 
          printf("EntryImageTermF second moment not implemented\n"); 
          break;
        default:
          break;
        }
      }
      ~EntryImageTermF() {}
    public:
      double evaluate() const {
	const EntryImage& e = *static_cast<const EntryImage*>(_entry);
	const DescImage& d = e.desc();
        const ImageMask* mask = d.mask();
	double s0 = 0, sum = 0, sqsum = 0;
	const double p = e.info(EntryImage::Pedestal);
	int ixlo, ixhi, iylo, iyhi;
        if (mask) {
	  if (d.rphi_bounds(ixlo, ixhi, iylo, iyhi,
			    _xc, _yc, _r1)) {
	    double xc(_xc), yc(_yc);
	    double r0sq(_r0*_r0), r1sq(_r1*_r1);
	    for(int j=iylo; j<=iyhi; j++) {
              if (!mask->row(j)) continue;
	      double dy  = d.biny(j)-yc;
	      double dy2 = dy*dy;
	      for(int i=ixlo; i<=ixhi; i++) {
                if (!mask->rowcol(j,i)) continue;
		double dx  = d.binx(i)-xc;
		double dx2 = dx*dx;
		double rsq = dx2 + dy2;
		double f   = atan2(dy,dx);
		if ( (rsq >= r0sq && rsq <= r1sq) &&
		     ( (f>=_f0 && f<=_f1) ||
		       (f+2*M_PI <= _f1) ) ) {
                  double v = double(e.content(i,j))-p;
                  s0    += 1;
                  sum   += v;
                  sqsum += v*v;
                }
	      }
	    }
	  }
        }
	else if (d.nframes()) {
	  for(unsigned fn=0; fn<d.nframes(); fn++)
	    if (d.rphi_bounds(ixlo, ixhi, iylo, iyhi,
			      _xc, _yc, _r1, fn)) {
	      double xc(_xc), yc(_yc);
	      double r0sq(_r0*_r0), r1sq(_r1*_r1);
	      for(int j=iylo; j<=iyhi; j++) {
		double dy  = d.biny(j)-yc;
		double dy2 = dy*dy;
		for(int i=ixlo; i<=ixhi; i++) {
		  double dx  = d.binx(i)-xc;
		  double dx2 = dx*dx;
		  double rsq = dx2 + dy2;
		  double f   = atan2(dy,dx);
		  if ( (rsq >= r0sq && rsq <= r1sq) &&
		       ( (f>=_f0 && f<=_f1) ||
			 (f+2*M_PI <= _f1) ) ) {
                    double v = double(e.content(i,j))-p;
                    s0    += 1;
                    sum   += v;
                    sqsum += v*v;
                  }
		}
	      }
	    }
	}
	else {
	  if (d.rphi_bounds(ixlo, ixhi, iylo, iyhi,
			    _xc, _yc, _r1)) {
	    double xc(_xc), yc(_yc);
	    double r0sq(_r0*_r0), r1sq(_r1*_r1);
	    for(int j=iylo; j<=iyhi; j++) {
	      double dy  = d.biny(j)-yc;
	      double dy2 = dy*dy;
	      for(int i=ixlo; i<=ixhi; i++) {
		double dx  = d.binx(i)-xc;
		double dx2 = dx*dx;
		double rsq = dx2 + dy2;
		double f   = atan2(dy,dx);
		if ( (rsq >= r0sq && rsq <= r1sq) &&
		     ( (f>=_f0 && f<=_f1) ||
		       (f+2*M_PI <= _f1) ) ) {
                  double v = double(e.content(i,j))-p;
                  s0    += 1;
                  sum   += v;
                  sqsum += v*v;
                }
	      }
	    }
	  }
          // 	  else {
          //             return 0;
          //          }
	}
	double n = double(e.info(EntryImage::Normalization));
        double v;
        switch(_mom) {
        case Zero:
          v = sum;
          if (n > 0) v/=n;
          break;
        case Contrast:
          v = sqrt(s0*sqsum/(sum*sum) - 1);
          if ( n > 0) v/=sqrt(n);
          break;
        default:
          v = 0;
          break;
        }
        return v;
      }
    private:
      const Entry*& _entry;
      double _xc, _yc, _r0, _r1, _f0, _f1;
      Moment _mom;
    };

  };
};

using namespace Ami;

static QChar _integrate(0x002C);  // ,
static QChar _moment1  (0x0027);  // '
static QChar _moment2  (0x0022);  // "
static QChar _contrast (0x0021);  // !
static QChar _range    (0x0023);  // #
const QChar& BinMath::integrate() { return _integrate; }
const QChar& BinMath::moment1  () { return _moment1  ; }
const QChar& BinMath::moment2  () { return _moment2  ; }
const QChar& BinMath::contrast () { return _contrast ; }
const QChar& BinMath::range    () { return _range    ; }
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
    QRegExp match("\\[[0-9!,\\\'\\\"]+\\]");
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
      o.type() == DescEntry::Scan) {
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
    case DescEntry::TH1F:
      { EntryTH1F* en = static_cast<EntryTH1F*  >(_entry);
	en->addcontent(1.,y); 
	en->addinfo(1.,EntryTH1F::Normalization);
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
    case DescEntry::TH2F:
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
    if ((index=use.indexOf(_moment1)) == -1)
      if ((index=use.indexOf(_moment2)) == -1) 
        if ((index=use.indexOf(_contrast)) == -1) {
          mom = None;
          lo = hi = use.toInt();
          return lo <  MAX_INDEX;
        }
        else
          mom = Contrast;
      else
        mom = Second;
    else 
      mom = First;
  else
    mom = Zero;

  lo = use.mid(0,index).toInt();
  hi = use.mid(index+1,-1).toInt();

  return true;
}

