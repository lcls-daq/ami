#ifndef AmiQt_FrameClient_hh
#define AmiQt_FrameClient_hh

#include "ami/qt/ImageClient.hh"

class QCheckBox;

namespace Ami {
  namespace Qt {
    class FrameClient : public ImageClient {
      Q_OBJECT
    public:
      FrameClient(QWidget*,const Pds::DetInfo&, unsigned, const QString&);
      ~FrameClient();
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
      
