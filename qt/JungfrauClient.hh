#ifndef AmiQt_JungfrauClient_hh
#define AmiQt_JungfrauClient_hh

#include "ami/qt/ImageClient.hh"

class QCheckBox;

namespace Ami {
  namespace Qt {
    class JungfrauClient : public ImageClient {
      Q_OBJECT
    public:
      JungfrauClient(QWidget*,const Pds::DetInfo&, unsigned, const QString&);
      ~JungfrauClient();
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
      void retain_corrections(bool);
    private:
      //  Specialization widgets
      QCheckBox* _npBox;
      bool _reloadPedestals;
    };
  };
};

#endif
      
