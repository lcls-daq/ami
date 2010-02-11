#ifndef AmiQt_DetectorGroup_hh
#define AmiQt_DetectorGroup_hh

#include "ami/qt/QtPWidget.hh"
#include <list>

class QButtonGroup;
class QVBoxLayout;

namespace Ami {
  namespace Qt {
    class QtTopWidget;

    class DetectorGroup : public QtPWidget {
      Q_OBJECT
    public:
      DetectorGroup(const QString&,
		    QWidget* parent,
		    const std::list<QtTopWidget*>&);
      ~DetectorGroup();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      void enable (int);
      void disable(int);
    public slots:
      void update_list();
      void apply();
      void close();
    private:
      virtual void _init () {}
      virtual void _apply(QtTopWidget&) = 0;
    private:
      const std::list<QtTopWidget*>& _clients;
      QVBoxLayout*                   _client_layout;
      std::list<QtTopWidget*>        _snapshot;
      QButtonGroup* _buttons;
    };
  };
};

#endif
