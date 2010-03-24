#ifndef AmiQt_ScalarPlotDesc_hh
#define AmiQt_ScalarPlotDesc_hh

#include <QtGui/QWidget>

class QButtonGroup;
class QString;

namespace Ami {
  class DescEntry;
  namespace Qt {
    class DescTH1F;
    class DescChart;
    class DescProf;
    class DescScan;

    class ScalarPlotDesc : public QWidget {
    public:
      enum Type { TH1F, vT, vF, vS };
      ScalarPlotDesc(QWidget* parent);
      ~ScalarPlotDesc();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      DescEntry* desc(const char*) const;
    private:
      QButtonGroup* _plot_grp;
      DescTH1F*   _hist;
      DescChart*  _vTime;
      DescProf*   _vFeature;
      DescScan*   _vScan;
    };
  };
};

#endif
