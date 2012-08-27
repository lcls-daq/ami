#ifndef AmiQt_QtPWidget_hh
#define AmiQt_QtPWidget_hh

#include "QtPersistent.hh"

//#define USE_DIALOG

#ifdef USE_DIALOG
#include <QtGui/QDialog>
#else
#include <QtGui/QWidget>
#endif

namespace Ami {
  namespace Qt {
#ifdef USE_DIALOG
    class QtPWidget : public QDialog {
#else
    class QtPWidget : public QWidget { // , public QtPersistent {
#endif
      Q_OBJECT
    public:
      QtPWidget();
      QtPWidget(QWidget* parent);
      virtual ~QtPWidget();
    public:
      virtual void save(char*& p) const;
      virtual void load(const char*& p);
    public slots:
      void front();
    };
  };
};

#endif
