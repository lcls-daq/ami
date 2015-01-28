#ifndef AmiQt_QtPWidget_hh
#define AmiQt_QtPWidget_hh

#include "ami/data/QtPersistent.hh"

#include <QtGui/QWidget>

namespace Ami {
  namespace Qt {
    class QtPWidget : public QWidget { // , public QtPersistent {
      Q_OBJECT
    public:
      QtPWidget();
      QtPWidget(int);
      QtPWidget(QWidget* parent);
      virtual ~QtPWidget();
    public:
      virtual void save(char*& p) const;
      virtual void load(const char*& p);
    public slots:
      void front();
    protected:
      void closeEvent(QCloseEvent*);
    signals:
      void opened();
      void closed(QObject*);
      void closed();
    private:
      QWidget* _parent;
    protected:
      bool _initial_hide;
    };
  };
};

#endif
