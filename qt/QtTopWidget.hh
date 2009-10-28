#ifndef AmiQt_QtTopWidget_hh
#define AmiQt_QtTopWidget_hh

#include "QtPWidget.hh"

namespace Ami {
  namespace Qt {
    class QtTopWidget : public QtPWidget {
    public:
      QtTopWidget(QWidget* parent) : QtPWidget(parent) {}
      virtual ~QtTopWidget() {}
    public:
      virtual void save_setup(char*& p) const { save(p); }
      virtual void load_setup(const char*& p) { load(p); }
      virtual void save_plots(const QString&) const = 0;
      virtual void reset_plots() = 0;
    };
  };
};

#endif
