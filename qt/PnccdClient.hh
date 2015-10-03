#ifndef AmiQt_PnccdClient_hh
#define AmiQt_PnccdClient_hh

#include "ami/qt/ImageClient.hh"

#include "ami/data/DescImage.hh"

class QCheckBox;
class QComboBox;

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
    protected:
      Rotation rotation() const;
    private:
      //  Specialization widgets
      PnccdCalibrator* _calibrator;
      QCheckBox* _fnBox;
      QCheckBox* _npBox;
      QComboBox* _roBox;
      bool _reloadPedestals;

      friend class PnccdCalibrator;
    };
  };
};

#endif
      
