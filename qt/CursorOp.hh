#ifndef AmiQt_CursorOp_hh
#define AmiQt_CursorOp_hh

#include "ami/data/ConfigureRequest.hh"
#include "ami/data/ConfigureRequestor.hh"

#include <map>
#include <string>

namespace Ami {
  class BinMath;
  namespace Qt {
    class AxisInfo;
    class ChannelDefinition;
    class CursorOp {
    public:
      CursorOp() {}
      ~CursorOp() {}
    public:
      void configure(char*& p, unsigned input, unsigned& output,
                     const AxisInfo&, ConfigureRequest::Source);
      void configure(char*& p, unsigned input, unsigned& output,
                     const AxisInfo&, ConfigureRequest::Source,
		     const std::map<std::string,double>&);
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels,
		     const AxisInfo&, ConfigureRequest::Source);
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels,
		     const AxisInfo&, ConfigureRequest::Source,
		     const std::map<std::string,double>&);
    public:
      unsigned channel() { return _channel; }
    protected:
      unsigned _channel;
      BinMath* _input;
      unsigned _output_signature;
      ConfigureRequestor _req;
    };
  };
};

#endif
