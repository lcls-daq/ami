#ifndef AmiQt_ScalarPlotDesc_hh
#define AmiQt_ScalarPlotDesc_hh

#include "ami/qt/FeatureList.hh"
#include "ami/qt/FeatureRegistry.hh"

#include <QtGui/QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QButtonGroup;
class QTabWidget;
class QCheckBox;
class QString;

namespace Ami {
  class DescEntry;
  namespace Qt {
    class DescTH1F;
    class DescTH2F;
    class DescChart;
    class DescProf;
    class DescProf2D;
    class DescScan;
    //    class DescText;

    class ScalarPlotDesc : public QWidget {
      Q_OBJECT
    public:
      enum Type { TH1F, vT, vF, vS, TH2F, vF2 };
      ScalarPlotDesc(QWidget* parent, 
                     FeatureRegistry* registry = &FeatureRegistry::instance(),
                     bool lNormWeight=true);
      ~ScalarPlotDesc();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      void post(QObject*, const char*);
    public:
      const char*     title() const;
      QString         qtitle() const;
      DescEntry*  desc(const char*) const;
      const char* expr(const QString& e) const;
      bool        postAnalysis() const;
    public slots:
      void aggClicked(int);
      void update_interval();
    protected:
      QLineEdit*   _title;
      QPushButton* _postB;

      QCheckBox*    _xnorm;
      QCheckBox*    _ynorm;
      FeatureList*   _vnorm;

      QCheckBox*    _weightB;
      FeatureList*  _vweight;

      QTabWidget* _plot_grp;
      DescTH1F*   _hist;
      DescChart*  _vTime;
      DescProf*   _vFeature;
      DescProf2D* _vFeature2;
      DescScan*   _vScan;
      DescTH2F*   _hist2d;
      //      DescText*   _text;

      QButtonGroup* _agg_grp;
      QLineEdit*    _interval;
      QLabel*       _intervalq;
    };
  };
};

#endif
