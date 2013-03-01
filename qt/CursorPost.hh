#ifndef AmiQt_CursorPost_hh
#define AmiQt_CursorPost_hh

#include "ami/qt/SharedData.hh"

#include "ami/data/ConfigureRequest.hh"
#include "ami/data/ConfigureRequestor.hh"

namespace Ami {
  class Cds;
  class DescEntry;
  class BinMath;
  namespace Qt {
    class AxisInfo;
    class ChannelDefinition;
    class CursorDefinition;
    class CPostParent;
    class CursorPost : public SharedData {
    public:
      CursorPost(unsigned       channel,
		 BinMath*       desc,
                 CPostParent*   parent =0);
      CursorPost(const char*&   p);
      ~CursorPost();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     const AxisInfo&, ConfigureRequest::Source);
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels,
		     const AxisInfo&, ConfigureRequest::Source);
    private:
      unsigned _channel;
      unsigned _output_signature;
      BinMath* _input;
      ConfigureRequestor _req;
      CPostParent*       _parent;
    };
  };
};

#endif
		 
