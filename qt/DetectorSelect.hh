#ifndef AmiQt_DetectorSelect_hh
#define AmiQt_DetectorSelect_hh

#include "ami/qt/QtPWidget.hh"

#include <QtCore/QString>

#include "pdsdata/xtc/DetInfo.hh"

class QPrinter;

namespace Ami {
  namespace Qt {
    class QtTopWidget;
    class DetectorReset;
    class DetectorSave;
    class DetectorSelect : public QtPWidget {
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
      void save_setup();
      void load_setup();
      void print_setup();

      void reset_plots();
      void save_plots();

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
      QtTopWidget** _client;
      char*       _restore;
      QPrinter*   _printer;
      DetectorReset*   _reset_box;
      DetectorSave*    _save_box;
    };
  };
};

#endif
