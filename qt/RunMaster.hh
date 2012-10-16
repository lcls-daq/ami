#ifndef AmiQt_RunMaster_hh
#define AmiQt_RunMaster_hh

#include <QtCore/QString>

namespace Ami {
  class Entry;
  class EntryScalar;

  namespace Qt {
    class RunMaster {
    public:
      static RunMaster* instance();
    public:
      void set_entry(Entry*);
      void update   ();
      void reset    ();
    public:
      QString run_title() const;
    private:
      RunMaster();
      ~RunMaster();
    private:
      QString _run_title;
      Ami::EntryScalar*  _entry;
    };
  };
};

#endif
