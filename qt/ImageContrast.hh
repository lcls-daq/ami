#ifndef AmiQt_ImageContrast_hh
#define AmiQt_ImageContrast_hh

#include "ami/qt/ScalarPlotDesc.hh"

class QLineEdit;

namespace Ami {
  namespace Qt {
    class ImageContrast : public ScalarPlotDesc {
    public:
      ImageContrast(QWidget* parent);
      ~ImageContrast();
    public:
      const char* expression() const;
    public:
      void update_range (int x1, int y1, int x2, int y2);
      void update_range (double xc, double yc,
                         double r1, double r2,
                         double f0, double f1);
    private:
      QLineEdit* _expr_edit;
    };
  };
};

#endif
