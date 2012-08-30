#ifndef AmiQt_DescTH1F_hh
#define AmiQt_DescTH1F_hh

#include <QtGui/QWidget>

class QRadioButton;
class QLineEdit;
class QComboBox;

namespace Ami {
  namespace Qt {
    class DescTH1F : public QWidget {
      Q_OBJECT
    public:
      DescTH1F(const char* name);
    public:
      void save(char*&) const;
      void load(const char*&);
    public:
      QRadioButton* button();
      enum Binning { Fixed, Auto1, Auto2 };
      Binning  method() const;
      unsigned bins  () const;
      double   lo    () const;
      double   hi    () const;
      double   sigma () const;
      double   extent() const;
      unsigned nsamples() const;
      void method(Binning);
      void bins  (unsigned);
      void lo    (double);
      void hi    (double);
      void sigma (double);
      void extent(double);
      void nsamples(unsigned);
    public slots:
      void validate();
    private:
      QRadioButton* _button;
      QLineEdit *_bins, *_lo, *_hi, *_xsigma, *_xrange, *_nsigma, *_nrange;
      QComboBox* _method;
    };
  };
};

#endif      
