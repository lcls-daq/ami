#ifndef AmiQt_QtTable_hh
#define AmiQt_QtTable_hh

#include <QtGui/QWidget>

class QLabel;

namespace Ami {
  class EntryScalar;
  namespace Qt {

    class QtTable : public QWidget {
      Q_OBJECT
    public:
      QtTable(QObject*);
      ~QtTable();
    public:
      const EntryScalar* entry() const { return _entry; }
      void entry (const EntryScalar* e);
      void update();
    public slots:
      void remove();
    private:
      const EntryScalar* _entry;
      EntryScalar*       _cache;
      QLabel*            _label;
      QLabel*            _value;
    };
  };
};

#endif
