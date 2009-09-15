#ifndef AmiQt_DetectorSelect_hh
#define AmiQt_DetectorSelect_hh

#include <QtGui/QWidget>

#include "pdsdata/xtc/DetInfo.hh"

namespace Ami {
  namespace Qt {
    class DetectorSelect : public QWidget {
      Q_OBJECT
    public:
      DetectorSelect(unsigned interface,
		     unsigned serverGroup);
      ~DetectorSelect();
    public:
      void start_client(Pds::DetInfo::Detector, unsigned);
    public slots:
      void start_gd   ();
      void start_ims  ();
      void start_itof ();
      void start_mbes ();
      void start_etof (int);
    private:
      unsigned _interface;
      unsigned _serverGroup;
    };
  };
};

#endif
