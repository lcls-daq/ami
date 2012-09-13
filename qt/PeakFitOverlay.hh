#ifndef AmiQt_PeakFitOverlay_hh
#define AmiQt_PeakFitOverlay_hh

#include "ami/qt/QtOverlay.hh"

#include <QtCore/QString>

#include "ami/data/ConfigureRequest.hh"
#include "ami/data/ConfigureRequestor.hh"

#include <list>

class QwtPlot;

namespace Ami {
  class Cds;
  class DescEntry;
  class EntryScalarRange;
  class PeakFitPlot;
  namespace Qt {
    class AxisInfo;
    class ChannelDefinition;
    class QtBase;
    class PeakFitOverlay : public QtOverlay {
    public:
      PeakFitOverlay(OverlayParent& parent,
                     QtPlot&        plot,
                     unsigned       channel,
                     Ami::PeakFitPlot* desc);
      PeakFitOverlay(OverlayParent& parent,
                     const char*&   p);
      ~PeakFitOverlay();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels,
		     const AxisInfo&, ConfigureRequest::Source);
      void setup_payload(Cds&);
      void update();
      void dump(FILE*) const;
    private:
      void _attach();
    private:
      QtPlot*              _frame;
      QString              _frame_name;

      unsigned             _channel;
      Ami::PeakFitPlot*    _input;
      
      unsigned             _output_signature;

      QtBase*              _plot;
      ConfigureRequestor   _req;

      const EntryScalarRange* _auto_range;

      int                  _order;
    };
  };
};

#endif
		 
