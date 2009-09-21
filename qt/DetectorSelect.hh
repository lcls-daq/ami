#ifndef AmiQt_DetectorSelect_hh
#define AmiQt_DetectorSelect_hh

#include <QtGui/QWidget>

#include "pdsdata/xtc/DetInfo.hh"

namespace Ami {
  namespace Qt {
    class Client;
    class DetectorSelect : public QWidget {
      Q_OBJECT
    public:
      DetectorSelect(unsigned interface,
		     unsigned serverGroup);
      ~DetectorSelect();
    public:
      void start_waveform_client(Pds::DetInfo::Detector, unsigned, unsigned);
      void start_image_client(Pds::DetInfo::Detector, unsigned, unsigned);
    public slots:
      void start_gd   ();
      void start_ims  ();
      void start_itof ();
      void start_mbes ();
      void start_etof (int);
      void start_bps  (int);
      void start_vmi  ();
    private:
      unsigned _interface;
      unsigned _serverGroup;
      enum { MaxClients=12 };
      Client* _client[MaxClients];
    };
  };
};

#endif
