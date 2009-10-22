#ifndef AmiQt_DetectorSelect_hh
#define AmiQt_DetectorSelect_hh

#include <QtGui/QWidget>
#include <QtCore/QString>

#include "pdsdata/xtc/DetInfo.hh"

class QPrinter;

namespace Ami {
  namespace Qt {
    class QtPWidget;
    class DetectorSelect : public QWidget {
      Q_OBJECT
    public:
      DetectorSelect(const QString&,
		     unsigned interface,
		     unsigned serverGroup);
      ~DetectorSelect();
    public:
      void start_waveform_client(Pds::DetInfo::Detector, unsigned, unsigned);
      void start_image_client(Pds::DetInfo::Detector, unsigned, unsigned);
      void start_features_client(unsigned);
    public slots:
      void save();
      void load();
      void print_setup();
      void start_gd   (int);
      void start_ims  ();
      void start_itof ();
      void start_mbes (int);
      void start_etof (int);
      void start_bps  (int);
      void start_vmi  ();
      void start_env  ();
    private:
      unsigned _interface;
      unsigned _serverGroup;
      QtPWidget** _client;
      char*       _restore;
      QPrinter*   _printer;
    };
  };
};

#endif
