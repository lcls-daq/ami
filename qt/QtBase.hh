#ifndef Ami_QtBase_hh
#define Ami_QtBase_hh

#include <QtCore/QString>
#include <QtGui/QColor>

class QwtPlot;

#include <stdio.h>

namespace Ami {
  class Entry;
  namespace Qt {
    class AxisInfo;
    class ImageFrame;
    class QtBase {
    public:
      QtBase(const QString& title,
	     const Ami::Entry&   entry) : _title(title), _entry(&entry) {}
      virtual ~QtBase() {}
    public:
      virtual void        dump  (FILE*) const = 0;
      virtual void        attach(QwtPlot*) {};
      virtual void        attach(ImageFrame*) {};
    public:
      virtual void        update()          = 0;
      virtual void        xscale_update()   = 0;
      virtual void        yscale_update()   = 0;
      virtual const AxisInfo* xinfo() const = 0;
      virtual const AxisInfo* yinfo() const { return 0; }
      virtual double      normalization() const { return 0; }
      virtual void        set_color(unsigned) {}
      virtual QColor      get_color() const;
    public:
      const QString& title() const { return _title; }
      const Ami::Entry& entry() const { return *_entry; }
      void              entry(const Ami::Entry& e) { _entry=&e; }
    public:
      static const QColor& color(unsigned);
    private:
      QString _title;
      const Ami::Entry* _entry;
    };
  };
};

#endif
