#include "FeatureExpression.hh"
#include "ami/data/FeatureCache.hh"

#include <QtCore/QRegExp>

using namespace Ami;

Feature::Feature(FeatureCache& f, unsigned index) :
  _features(f),
  _index   (index)
{
}

Feature::~Feature() {}

double Feature::evaluate() const
{
  bool dmg;
  double v = _features.cache(_index,&dmg);
  if (dmg) damage(true);
  return v;
}

static bool _dmg;
void Feature::damage(bool l) { _dmg = l; }
bool Feature::damage() { return _dmg; }


FeatureExpression::FeatureExpression() : Expression(_variables) {}

FeatureExpression::~FeatureExpression() {}

Term* FeatureExpression::evaluate(FeatureCache& features,
				  const QString& expr)
{
  QString new_expr;

  {  // parse expression for FeatureCache indices
    QRegExp match("\\[[0-9]+\\]");
    int last=0;
    int pos=0;
    while( (pos=match.indexIn(expr,pos)) != -1) {
      QString use = expr.mid(pos+1,match.matchedLength()-2);
      unsigned index = use.toInt();
      Term* t = new Feature(features,index);
      new_expr.append(expr.mid(last,pos-last));
      new_expr.append(QString("[%1]").arg((ulong)t,0,16));
      pos += match.matchedLength();
      last = pos;
    }
    new_expr.append(expr.mid(last));
  }

  return Expression::evaluate(new_expr);
}
