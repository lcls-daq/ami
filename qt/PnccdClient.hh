#ifndef AmiQt_PnccdClient_hh
#define AmiQt_PnccdClient_hh

#include "ami/qt/ImageClient.hh"

class QCheckBox;

namespace Ami {
  namespace Qt {
    class PnccdClient : public ImageClient {
      Q_OBJECT
    public:
      PnccdClient(QWidget*,const Pds::DetInfo&, unsigned);
      ~PnccdClient();
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
      QCheckBox* _fnBox;
      QCheckBox* _npBox;
      bool _reloadPedestals;
    };
  };
};

#endif
      
