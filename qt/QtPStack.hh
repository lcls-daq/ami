#ifndef AmiQt_QtPStack_hh
#define AmiQt_QtPStack_hh

#include <QtGui/QStackedWidget>

class QPushButton;

namespace Ami {
  namespace Qt {
    class QtPWidget;
    class QtPStack : public QStackedWidget {
      Q_OBJECT
    public:
      QtPStack();
      virtual ~QtPStack();
    public:
      void add(QPushButton*, QtPWidget*);
    public:
      static void attach(bool);
    signals:
      void hidden();
    public slots:
      void setPWidget();
      void resetPWidget();
    };
  };
};

#endif
