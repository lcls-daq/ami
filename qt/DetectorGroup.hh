#ifndef AmiQt_DetectorGroup_hh
#define AmiQt_DetectorGroup_hh

#include "ami/qt/QtPWidget.hh"

class QButtonGroup;

namespace Ami {
  namespace Qt {
    class QtTopWidget;

    class DetectorGroup : public QtPWidget {
      Q_OBJECT
    public:
      DetectorGroup(const QString&,
		    QWidget* parent,
		    QtTopWidget**,
		    const char**,
		    int);
      ~DetectorGroup();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      void enable (int);
      void disable(int);
    public slots:
      void apply();
      void close();
    private:
      virtual void _init () {}
      virtual void _apply(QtTopWidget&,const QString&) = 0;
    private:
      QtTopWidget** _clients;
      int           _n;
      QButtonGroup* _buttons;
    };
  };
};

#endif
