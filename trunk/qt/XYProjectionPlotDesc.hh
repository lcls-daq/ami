#ifndef AmiQt_XYProjectionPlotDesc_hh
#define AmiQt_XYProjectionPlotDesc_hh

#include <QtGui/QWidget>

class QButtonGroup;

namespace Ami {
  class DescEntry;
  class XYProjection;
  namespace Qt {
    class XYProjectionPlotDesc : public QWidget {
    public:
      XYProjectionPlotDesc(QWidget* parent);
      ~XYProjectionPlotDesc();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      Ami::XYProjection* desc(const char*) const;
    private:
      QButtonGroup* _axis;
      QButtonGroup* _norm;
    };
  };
};

#endif
