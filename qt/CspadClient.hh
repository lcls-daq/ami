#ifndef AmiQt_CspadClient_hh
#define AmiQt_CspadClient_hh

#include "ami/qt/ImageClient.hh"

#include "ami/data/DescImage.hh"

class QCheckBox;

namespace Ami {
  namespace Qt {
    class Rotator;
    class CspadClient : public ImageClient {
      Q_OBJECT
    public:
      CspadClient(QWidget*,const Pds::DetInfo&, unsigned, const QString&);
      ~CspadClient();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      Ami::Rotation rotation() const;
    protected:
      void     _prototype   (const DescEntry&);
      unsigned _preconfigure(char*&    p,
          unsigned  input,
          unsigned& output,
          ConfigureRequest::Source&);
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
      QCheckBox* _fnBox;
      QCheckBox* _unBox;
      QCheckBox* _spBox;
      QCheckBox* _npBox;
      QCheckBox* _gnBox;
      QCheckBox* _piBox;
      Rotator*   _rotator;
      bool _reloadPedestals;
    };
  };
};

#endif
      
