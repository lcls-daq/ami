#ifndef AmiQt_ImageClient_hh
#define AmiQt_ImageClient_hh

#include "ami/qt/Client.hh"

namespace Ami {
  namespace Qt {
    class ImageXYProjection;
    class ImageRPhiProjection;
    class ImageContourProjection;
    class PeakFinder;
    class BlobFinder;
    class Droplet;

    class ImageClient : public Client {
    public:
      ImageClient(QWidget*,const Pds::DetInfo&, unsigned, const QString&);
      ~ImageClient();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void snapshot(const QString&) const;
      void save_plots(const QString&) const;
    protected:
      void _configure(char*& p, 
		      unsigned input, 
		      unsigned& output,
		      ChannelDefinition* ch[], 
		      int* signatures, 
		      unsigned nchannels);
      void _setup_payload(Cds&);
      void _update();
      void _prototype(const DescEntry&);
      
    private:
      ImageXYProjection*      _xyproj;
      ImageRPhiProjection*    _rfproj;
      ImageContourProjection* _cntproj;
      PeakFinder*             _hit;
      BlobFinder*             _blob;
      Droplet*                _droplet;
    };
  };
};

#endif
