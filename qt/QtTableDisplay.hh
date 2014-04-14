#ifndef AmiQt_QtTableDisplay_hh
#define AmiQt_QtTableDisplay_hh

#include <QtGui/QWidget>

namespace Ami {
  namespace Qt {
    class QtTable;
    class QtTableDisplay : public QWidget {
      Q_OBJECT
    public:
      QtTableDisplay();
      ~QtTableDisplay();
    public:
      void add   (QtTable&);
    public slots:
      void update();
    protected:
      void paintEvent(QPaintEvent*);
    public:
      static QtTableDisplay* instance();
    private:
      bool _size_changed;
    };
  };
};

#endif
