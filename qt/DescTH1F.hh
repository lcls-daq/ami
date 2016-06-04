#ifndef AmiQt_DescTH1F_hh
#define AmiQt_DescTH1F_hh

#include "ami/qt/DescBinning.hh"

class QRadioButton;
class QCheckBox;
class QButtonGroup;

namespace Ami {
  namespace Qt {
    class DescTH1F : public DescBinning {
    public:
      DescTH1F(const char* name, bool autoRange=true, bool normalize=true, bool aggregate=false);
      ~DescTH1F();
    public:
      QRadioButton* button();
      bool          normalize() const;
      bool          aggregate() const;
    public:
      void save(char*&) const;
      void load(const char*&);
    private:
      QRadioButton* _button;
      QCheckBox*    _normalize;
      QButtonGroup* _aggregate_grp;
    };
  };
};

#endif      
