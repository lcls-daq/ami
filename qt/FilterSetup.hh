#ifndef AmiQt_FilterSetup_hh
#define AmiQt_FilterSetup_hh

#include "ami/qt/QtPWidget.hh"

class QListWidget;
class QListWidgetItem;

namespace Ami {
  class ClientManager;
  class DiscoveryRx;
  class AbsFilter;

  namespace Qt {
    class Filter;
    class FilterSetup : public QtPWidget {
      Q_OBJECT
    public:
      FilterSetup(ClientManager&);
      ~FilterSetup();
    public:
      void                  update(const DiscoveryRx&);
      unsigned              selected() const;
      const Ami::AbsFilter& filter() const;
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public slots:
      void activated(QListWidgetItem*);
      void apply();
    private:
      ClientManager& _manager;
      QListWidget*   _list;
      Filter*        _filter;
    };
  };
};

#endif
