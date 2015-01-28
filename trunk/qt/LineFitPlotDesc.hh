#ifndef AmiQt_LineFitPlotDesc_hh
#define AmiQt_LineFitPlotDesc_hh

#include "ami/qt/FeatureList.hh"
#include "ami/qt/FeatureRegistry.hh"

#include <QtGui/QWidget>

class QComboBox;
class QTabWidget;

namespace Ami {
  class DescEntry;
  namespace Qt {
    class DescChart;
    class DescProf;
    class DescProf2D;
    class DescScan;

    class LineFitPlotDesc : public QWidget {
      Q_OBJECT
    public:
      enum Type { vT, vF, vS, vF2 };
      LineFitPlotDesc(QWidget* parent, 
		      FeatureRegistry* registry = &FeatureRegistry::instance());
      ~LineFitPlotDesc();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      DescEntry*      desc        (const char*) const;
      DescEntry*      table       (const char*) const;
      QString         stat        () const;
      bool            postAnalysis() const;
    protected:
      QComboBox*  _method;
      QComboBox*  _stat;
      QTabWidget* _plot_grp;
      DescChart*  _vTime;
      DescProf*   _vFeature;
      DescProf2D* _vFeature2;
      DescScan*   _vScan;
    };
  };
};

#endif
