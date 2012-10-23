#ifndef AmiQt_DescTH2F_hh
#define AmiQt_DescTH2F_hh

#include <QtGui/QWidget>

#include "ami/qt/DescBinning.hh"

class QRadioButton;
class QButtonGroup;
class QLineEdit;

namespace Ami {
  namespace Qt {
    class FeatureRegistry;
    class DescTH2F : public QWidget {
      Q_OBJECT
    public:
      DescTH2F(const char*, FeatureRegistry*);
    public:
      void save(char*&) const;
      void load(const char*&);
    public slots:
      void calc();
    public:
      QRadioButton* button();
      enum Output { TH2F, Image };
      Output output() const;
      const DescBinning& xbins() const;
      const DescBinning& ybins() const;
      QString expr() const;
    private:
      QRadioButton* _button;  // vestigial
      QButtonGroup* _group;
      DescBinning*  _xbins;
      DescBinning*  _ybins;
      QLineEdit*    _expr;
      FeatureRegistry* _registry;
    };
  };
};

#endif      
