#ifndef AmiQt_CurveFitOverlay_hh
#define AmiQt_CurveFitOverlay_hh

#include "ami/qt/QtOverlay.hh"
#include "ami/data/ConfigureRequestor.hh"

#include <QtCore/QString>

#include <list>

namespace Ami {
  class Cds;
  class DescEntry;
  class CurveFit;
  namespace Qt {
    class AxisInfo;
    class ChannelDefinition;
    class CursorDefinition;
    class QtBase;
    class CurveFitOverlay : public QtOverlay {
    public:
      CurveFitOverlay(OverlayParent&   parent,
                      QtPlot&          plot,
                      unsigned         channel,
                      Ami::CurveFit*   fit);
      CurveFitOverlay(OverlayParent&   parent,
                      const char*&     p);
      ~CurveFitOverlay();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      void configure(char*& p, unsigned input, unsigned& output,
                     ChannelDefinition* ch[], int* signatures, unsigned nchannels,
                     const AxisInfo&);
      void setup_payload(Cds&);
      void update();
      void dump(FILE*) const;
    private:
      void savefit(char*& p) const;
      Ami::CurveFit *loadfit(const char*& p);
    private:
      void _attach();
    private:
      QtPlot*            _frame;
      QString            _frame_name;
      unsigned           _channel;
      Ami::CurveFit*     _fit;
      unsigned           _output_signature;
      QtBase*            _plot;
      ConfigureRequestor _req;
      int                _order;
    };
  };
};

#endif
