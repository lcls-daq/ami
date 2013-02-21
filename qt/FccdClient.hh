#ifndef AmiQt_FccdClient_hh
#define AmiQt_FccdClient_hh

#include "ami/qt/ImageClient.hh"

class QCheckBox;

namespace Ami {
  namespace Qt {
    class FccdClient : public ImageClient {
      Q_OBJECT
    public:
      FccdClient(QWidget*,const Pds::DetInfo&, unsigned);
      ~FccdClient();
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
      
