#ifndef AmiQt_CurveFitPost_hh
#define AmiQt_CurveFitPost_hh

#include "ami/qt/SharedData.hh"

#include "ami/data/CurveFit.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/ConfigureRequestor.hh"

namespace Ami {
  class Cds;
  class DescEntry;
  class CurveFit;
  namespace Qt {
    class AxisInfo;
    class ChannelDefinition;
    class CFPostParent;
    class CurveFitPost : public SharedData {
    public:
      CurveFitPost(unsigned          channel,
		   Ami::CurveFit*    input,
                   CFPostParent*     parent=0);
      CurveFitPost(const char*&   p);
      ~CurveFitPost();
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
      Ami::CurveFit* _input;
      ConfigureRequestor _req;
      CFPostParent*      _parent;
    };
  };
};

#endif
		 
