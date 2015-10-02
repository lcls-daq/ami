#ifndef AmiQt_PnccdClient_hh
#define AmiQt_PnccdClient_hh

#include "ami/qt/ImageClient.hh"

class QCheckBox;

namespace Ami {
  namespace Qt {
    class PnccdCalibrator;

    class PnccdClient : public ImageClient {
      Q_OBJECT
    public:
      PnccdClient(QWidget*,const Pds::DetInfo&, unsigned, const QString&);
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
      void _setup_payload(Cds&);
      void _update();
    private:
      //  Specialization widgets
      PnccdCalibrator* _calibrator;
      QCheckBox* _fnBox;
      QCheckBox* _npBox;
      QCheckBox* _roBox;
      bool _reloadPedestals;

      friend class PnccdCalibrator;
    };
  };
};

#endif
      
