#ifndef AmiQt_ScalarPlotDesc_hh
#define AmiQt_ScalarPlotDesc_hh

#include "ami/qt/FeatureList.hh"

#include <QtGui/QWidget>

class QLineEdit;
class QPushButton;
class QButtonGroup;
class QCheckBox;
class QString;

namespace Ami {
  class DescEntry;
  namespace Qt {
    class DescTH1F;
    class DescChart;
    class DescProf;
    class DescScan;
    class FeatureRegistry;

    class ScalarPlotDesc : public QWidget {
    public:
      enum Type { TH1F, vT, vF, vS };
      ScalarPlotDesc(QWidget* parent, FeatureRegistry* =0);
      ~ScalarPlotDesc();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      void post(QObject*, const char*);
    public:
      const char* title() const;
      DescEntry*  desc(const char*) const;
      const char* expr(const QString& e) const;
    protected:
      QLineEdit*   _title;
      QPushButton* _postB;

      QCheckBox*    _xnorm;
      QCheckBox*    _ynorm;
      FeatureList*   _vnorm;

      QCheckBox*    _weightB;
      FeatureList*  _vweight;

      QButtonGroup* _plot_grp;
      DescTH1F*   _hist;
      DescChart*  _vTime;
      DescProf*   _vFeature;
      DescScan*   _vScan;
    };
  };
};

#endif
