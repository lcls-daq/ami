#include "SelfExpression.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/BinMathTerms.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/DescWaveform.hh"
#include "ami/data/DescTH1F.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/DescImage.hh"

#include <QtCore/QRegExp>

namespace Ami {
  class SumTerm : public Term {
  public:
    SumTerm(const Entry*& entry, const DescEntry& desc) {

#define CASETERM(type) case DescEntry::type:                            \
      { const Desc##type& d = static_cast<const Desc##type&>(desc);     \
        _term = new BinMathC::Entry##type##Term(entry,                  \
                                                0, d.nbins()-1,         \
                                                Zero);                  \
        break; }

      switch(desc.type()) {
        CASETERM(Waveform)
        CASETERM(TH1F)
        CASETERM(Prof)
        case DescEntry::Image:
        { const DescImage& d = static_cast<const DescImage&>(desc);
          _term = new BinMathC::EntryImageTerm(entry, 
                                               0, d.nbinsx()-1,
                                               0, d.nbinsy()-1,
                                               Zero);
          break; }
      default:
        _term = 0;
        break;
      }
#undef CASETERM
    }
    ~SumTerm() { delete _term; }
  public:
    double evaluate() const { return _term->evaluate(); }
  private:
    Term*  _term;
  };
};

using namespace Ami;

SelfExpression::SelfExpression() {}

SelfExpression::~SelfExpression() {}

Term* SelfExpression::evaluate(FeatureCache& features,
                               const QString& e,
                               const Entry*& entry,
                               const DescEntry& d)
{
  QString expr(e);
  while(1) {
    int np = expr.indexOf(self_sum(),0);
    if (np >= 0) {
      Term* t = new SumTerm(entry, d);
      QString new_expr;
      new_expr.append(expr.mid(0,np));
      new_expr.append(QString("[%1]").arg((ulong)t,0,16));
      new_expr.append(expr.mid(np+self_sum().size()));
      
      expr = new_expr;
    }
    else
      break;
  }

  return FeatureExpression::evaluate(features,expr);
}

static QString _self_sum("SELF:SUM");

const QString& SelfExpression::self_sum() { return _self_sum; }
