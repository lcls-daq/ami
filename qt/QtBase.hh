#ifndef Ami_QtBase_hh
#define Ami_QtBase_hh

#include <QtCore/QString>

class QwtPlot;

#include <stdio.h>

namespace Ami {
  class Entry;
  namespace Qt {
    class AxisArray;
    class QtBase {
    public:
      QtBase(const QString& title,
	     const Ami::Entry&   entry) : _title(title), _entry(entry) {}
      virtual ~QtBase() {}
    public:
      virtual void        dump  (FILE*) const = 0;
      virtual void        attach(QwtPlot*) = 0;
    public:
      virtual void        update()          = 0;
      virtual void        xscale_update()   = 0;
      virtual void        yscale_update()   = 0;
      virtual const AxisArray* xinfo() const = 0;
//       virtual const AxisInfo* yinfo() const = 0;
    public:
      const QString& title() const { return _title; }
      const Ami::Entry& entry() const { return _entry; }
    private:
      QString _title;
      const Ami::Entry& _entry;
    };
  };
};

#endif
