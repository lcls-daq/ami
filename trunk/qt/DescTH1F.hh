#ifndef AmiQt_DescTH1F_hh
#define AmiQt_DescTH1F_hh

#include "ami/qt/DescBinning.hh"

class QRadioButton;
class QCheckBox;

namespace Ami {
  namespace Qt {
    class DescTH1F : public DescBinning {
    public:
      DescTH1F(const char* name, bool autoRange=true, bool normalize=true);
    public:
      QRadioButton* button();
      bool          normalize() const;
    public:
      void save(char*&) const;
      void load(const char*&);
    private:
      QRadioButton* _button;
      QCheckBox*    _normalize;
    };
  };
};

#endif      
