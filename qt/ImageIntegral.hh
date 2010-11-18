#ifndef AmiQt_ImageIntegral_hh
#define AmiQt_ImageIntegral_hh

#include "ami/qt/ScalarPlotDesc.hh"

class QLineEdit;

namespace Ami {
  namespace Qt {
    class ImageIntegral : public ScalarPlotDesc {
    public:
      ImageIntegral(QWidget* parent);
      ~ImageIntegral();
    public:
      const char* expression() const;
    public:
      void update_range (int x1, int y1, int x2, int y2);
    private:
      QLineEdit* _expr_edit;
    };
  };
};

#endif
