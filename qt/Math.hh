#ifndef AmiQt_Math_hh
#define AmiQt_Math_hh

#include <QtGui/QWidget>

class QLineEdit;
class QButtonGroup;
class QLabel;
class QPushButton;

namespace Ami {

  class AbsOperator;
  class Entry;

  namespace Qt {

    class DescTH1F;
    class DescProf;
    class DescChart;
    class Term;

    class Math : public QWidget {
      Q_OBJECT
    public:
      Math(const char*);
      ~Math();
    public:
      Ami::AbsOperator* math() const;
    public:
      const QPushButton* applyB() { return _applyB; }
    public slots:
      void load_ref();
      void reset_math();
      void sum_select();
      void apply();
    private:
      char    _name[64];
      QLabel* _ref;
      QLineEdit* _expr;
      QLineEdit* _sum_lo;
      QLineEdit* _sum_hi;
      QButtonGroup* _plot_grp;
      QPushButton*  _applyB;
      DescTH1F* _sum_rng;
      DescChart* _sumvt_rng;
      DescProf* _sumvbld_rng;
      DescProf* _sumvpv_rng;
      Ami::AbsOperator* _operator;
      Ami::Entry* _ref_entry;
      Term* _expression;
      unsigned _sum_xlo, _sum_xhi, _sum_ylo, _sum_yhi;
    };
  };
};

#endif
