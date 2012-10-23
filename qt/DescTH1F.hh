#ifndef AmiQt_DescTH1F_hh
#define AmiQt_DescTH1F_hh

#include "ami/qt/DescBinning.hh"

class QRadioButton;

namespace Ami {
  namespace Qt {
    class DescTH1F : public DescBinning {
    public:
      DescTH1F(const char* name);
    public:
      QRadioButton* button();
    private:
      QRadioButton* _button;
    };
  };
};

#endif      
