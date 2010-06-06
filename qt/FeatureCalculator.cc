#include "ami/qt/FeatureCalculator.hh"

#include "ami/qt/FeatureRegistry.hh"

using namespace Ami::Qt;

static QChar _exponentiate(0x005E);
static QChar _multiply    (0x00D7);
static QChar _divide      (0x00F7);
static QChar _add         (0x002B);
static QChar _subtract    (0x002D);

static QStringList ops = QStringList() << _exponentiate
					  << _multiply
					     << _divide
						<< _add   
						   << _subtract;

static QStringList vops;

FeatureCalculator::FeatureCalculator(const QString& name) :
  Calculator(name, "",
	     FeatureRegistry::instance().names(),
	     vops, ops)
{
}
