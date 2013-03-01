#ifndef AmiQt_CursorOverlay_hh
#define AmiQt_CursorOverlay_hh

#include "ami/qt/QtOverlay.hh"

#include "ami/data/BinMath.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/ConfigureRequestor.hh"

#include <QtCore/QString>

namespace Ami {
  class Cds;
  class DescEntry;
  class EntryScalarRange;
  namespace Qt {
    class AxisInfo;
    class ChannelDefinition;
    class QtBase;
    class QtPlot;
    class CursorOverlay : public QtOverlay {
    public:
      CursorOverlay(OverlayParent&   parent,
                    QtPlot&          frame,
                    unsigned         channel,
                    BinMath*         input);
      CursorOverlay(OverlayParent&   parent,
                    const char*&    p);
      ~CursorOverlay();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void dump(FILE*) const;
      const QtBase* base() const;
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     const AxisInfo&, ConfigureRequest::Source);
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels,
		     const AxisInfo&, ConfigureRequest::Source);
      void setup_payload(Cds&);
      void update();
    private:
      void _attach();
    private:
      QtPlot*            _frame;
      QString            _frame_name;
      unsigned           _channel;
      BinMath*           _input;
      unsigned           _output_signature;
      QtBase*            _plot;
      ConfigureRequestor _req;
      EntryScalarRange*  _auto_range;
      int                _order;
    };
  };
};

#endif
		 
