#ifndef AmiQt_QAggSelect_hh
#define AmiQt_QAggSelect_hh

#include <QtGui/QWidget>

class QButtonGroup;
class QLineEdit;
class QLabel;

namespace Ami {
  namespace Qt {
    class SMPWarning;

    class QAggSelect : public QWidget {
      Q_OBJECT
    public:
      QAggSelect();
      ~QAggSelect();
    public:
      bool     at_rate     () const;
      unsigned over_events () const;
      int      value       () const;
      bool     smp_prohibit() const;
    public:
      void     load(const char*&);
      void     save(char*&) const;
    public slots:
      void enable(bool);
      void update_interval();
    private:
      QButtonGroup* _group;
      QLineEdit*    _interval;
      QLabel*       _intervalq;
      SMPWarning*   _smp_warning;
      bool          _enabled;
    };
  };
};

#endif
     
