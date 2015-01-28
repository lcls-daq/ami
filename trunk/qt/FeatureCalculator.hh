#ifndef AmiQt_FeatureCalculator_hh
#define AmiQt_FeatureCalculator_hh

#include "ami/qt/Calculator.hh"

namespace Ami {
  namespace Qt {
    class FeatureRegistry;
    class FeatureCalculator : public Calculator {
    public:
      FeatureCalculator(QWidget* parent, const QString&     title, FeatureRegistry&);
      FeatureCalculator(QWidget* parent, const QString&     title, FeatureRegistry&,
                        const QStringList& vars, const QStringList& vars_help);
    public:
      QString result();
    };
  };
};
#endif
