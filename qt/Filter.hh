#ifndef AmiQt_Filter_hh
#define AmiQt_Filter_hh

#include <QtGui/QWidget>

class QComboBox;
class QButtonGroup;
class QLineEdit;
class QVBoxLayout;

namespace Ami {
  class AbsFilter;

  namespace Qt {
    class Condition;

    class Filter : public QWidget {
      Q_OBJECT
    public:
      Filter(const QString&);
      ~Filter();
    public:
      const Ami::AbsFilter* filter() const;
    public slots:
      void add   ();
      void remove(const QString&);
      void calc  ();
      void apply ();
      void clear ();
      void save  ();
      void load  ();
      void update_features();
    signals:
      void changed();
    private:
      QString    _name;
      QLineEdit* _expr;
      QLineEdit* _cond_name;
      QComboBox* _bld_box;
      QComboBox* _features;
      QLineEdit* _lo_rng;
      QLineEdit* _hi_rng;
      QVBoxLayout* _clayout;
      std::list<Condition*> _conditions;

      AbsFilter* _filter;
    };
  };
};

#endif
