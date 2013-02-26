#ifndef AmiQt_EdgeOverlay_hh
#define AmiQt_EdgeOverlay_hh

#include "ami/qt/QtOverlay.hh"
#include "ami/data/ConfigureRequestor.hh"

#include <QtCore/QString>

#include <list>

namespace Ami {
  class Cds;
  class DescEntry;
  class EdgeFinder;
  namespace Qt {
    class AxisInfo;
    class ChannelDefinition;
    class CursorDefinition;
    class QtBase;
    class EdgeOverlay : public QtOverlay {
    public:
      EdgeOverlay(OverlayParent&   parent,
                  QtPlot&          plot,
                  unsigned         channel,
                  Ami::EdgeFinder* finder);
      EdgeOverlay(OverlayParent&   parent,
                  const char*&     p);
      ~EdgeOverlay();
    public:
      void savefinder(Ami::EdgeFinder *f, char*& p) const;
      Ami::EdgeFinder *loadfinder(const char *& p);
      void save(char*& p) const;
      void load(const char*& p);
      const QtBase* base() const;
    public:
      void addfinder(Ami::EdgeFinder *f);
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels,
		     const AxisInfo&);
      void setup_payload(Cds&);
      void update();
      void dump(FILE* f, int idx) const;
      void dump(FILE* f) const;
    private:
      void _attach(int);
    private:
      QtPlot*            _frame;
      QString            _frame_name;
      unsigned    _channel;
#define MAX_FINDERS  2
      Ami::EdgeFinder* _finder[MAX_FINDERS];
      int      _fcnt;
      unsigned _output_signature;
      QtBase*  _plot[MAX_FINDERS];
      ConfigureRequestor _req[MAX_FINDERS];
      int                _order[MAX_FINDERS];
    };
  };
};

#endif
		 
