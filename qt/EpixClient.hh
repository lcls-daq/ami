#ifndef AmiQt_EpixClient_hh
#define AmiQt_EpixClient_hh

#include "ami/qt/ImageClient.hh"

class QCheckBox;

namespace Ami {
  namespace Qt {
    class EpixClient : public ImageClient {
      Q_OBJECT
    public:
      EpixClient(QWidget*,const Pds::DetInfo&, unsigned);
      ~EpixClient();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    protected:
      void _configure(char*& p, 
		      unsigned input, 
		      unsigned& output,
		      ChannelDefinition* ch[], 
		      int* signatures, 
		      unsigned nchannels);
      void _setup_payload(Cds&);
    public slots:
      void write_pedestals();
    private:
      //  Specialization widgets
      QCheckBox* _npBox;
      bool _reloadPedestals;
    };
  };
};

#endif
      