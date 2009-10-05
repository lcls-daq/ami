#ifndef AmiQt_WaveformClient_hh
#define AmiQt_WaveformClient_hh

#include "ami/qt/Client.hh"

namespace Ami {
  namespace Qt {
    class CursorsX;
    class EdgeFinder;

    class WaveformClient : public Client {
    public:
      WaveformClient(QWidget*,const Pds::DetInfo&, unsigned);
      ~WaveformClient();
    public:
      void save(char*& p) const;
      void load(const char*& p);
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
      EdgeFinder*        _edges;
      CursorsX*          _cursors;
    };
  };
};

#endif
