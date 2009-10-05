#ifndef AmiQt_DetectorSelect_hh
#define AmiQt_DetectorSelect_hh

#include <QtGui/QWidget>

#include "pdsdata/xtc/DetInfo.hh"

namespace Ami {
  namespace Qt {
    class QtPWidget;
    class DetectorSelect : public QWidget {
      Q_OBJECT
    public:
      DetectorSelect(unsigned interface,
		     unsigned serverGroup);
      ~DetectorSelect();
    public:
      void start_waveform_client(Pds::DetInfo::Detector, unsigned, unsigned);
      void start_image_client(Pds::DetInfo::Detector, unsigned, unsigned);
      void start_features_client(unsigned);
    public slots:
      void save();
      void load();
      void start_gd   ();
      void start_ims  ();
      void start_itof ();
      void start_mbes ();
      void start_etof (int);
      void start_bps  (int);
      void start_vmi  ();
      void start_env  ();
    private:
      unsigned _interface;
      unsigned _serverGroup;
      QtPWidget** _client;
      char*       _restore;
    };
  };
};

#endif
