#ifndef AmiQt_ImageClient_hh
#define AmiQt_ImageClient_hh

#include "ami/qt/Client.hh"

namespace Ami {
  namespace Qt {
    class ImageXYProjection;
    class ImageRPhiProjection;
    class PeakFinder;
    class ImageClient : public Client {
    public:
      ImageClient(const Pds::DetInfo&, unsigned);
      ~ImageClient();
    private:
      void _configure(char*& p, 
		      unsigned input, 
		      unsigned& output,
		      ChannelDefinition* ch[], 
		      int* signatures, 
		      unsigned nchannels);
      void _setup_payload(Cds&);
      void _update();
    private:
      ImageXYProjection*   _xyproj;
      ImageRPhiProjection* _rfproj;
      PeakFinder*          _hit;
    };
  };
};

#endif
