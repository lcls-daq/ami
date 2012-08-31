#ifndef AmiQt_PWidgetManager_hh
#define AmiQt_PWidgetManager_hh

#include <QtGui/QWidget>

class QComboBox;

namespace Ami {
  namespace Qt {
    class QtPWidget;

    class PWidgetManager : public QWidget {
      Q_OBJECT
    public:
      PWidgetManager(QWidget* parent=0);
      ~PWidgetManager();
    public:
      static void add   (QtPWidget*, QString&);
      static void remove(QtPWidget*);
    public:
      void sync();
    public slots:
      void selected(int);
    private:
      QComboBox* _box;
    };
  };
};

#endif
