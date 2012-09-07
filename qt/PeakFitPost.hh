#ifndef AmiQt_PeakFitPost_hh
#define AmiQt_PeakFitPost_hh

#include "ami/qt/SharedData.hh"

#include "ami/data/PeakFitPlot.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/ConfigureRequestor.hh"

namespace Ami {
  class Cds;
  class DescEntry;
  class PeakFit;
  namespace Qt {
    class AxisInfo;
    class ChannelDefinition;
    class PFPostParent;
    class PeakFitPost : public SharedData {
    public:
      PeakFitPost(unsigned          channel,
		  Ami::PeakFitPlot* desc,
                  PFPostParent*   parent =0);
      PeakFitPost(const char*&   p);
      ~PeakFitPost();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels,
		     const AxisInfo&, ConfigureRequest::Source);
    private:
      unsigned   _channel;
      unsigned   _output_signature;
      Ami::PeakFitPlot* _input;
      ConfigureRequestor _req;
      PFPostParent*      _parent;
    };
  };
};

#endif
		 
