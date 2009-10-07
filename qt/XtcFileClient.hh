#ifndef AmiQt_XtcFileClient_hh
#define AmiQt_XtcFileClient_hh

#include <QtGui/QWidget>
#include "ami/service/Routine.hh"

#include <list>

class QListWidget;

namespace Pds {
  class Dgram;
  class Xtc;
  class Sequence;
};

namespace Ami {

  class Task;
  class XtcClient;

  namespace Qt {
    class FileSelect;
    class XtcFileClient : public  QWidget,
			  public  Routine {
      Q_OBJECT
    public:
      XtcFileClient(Ami::XtcClient&  client);
      ~XtcFileClient();
    public:
      void routine();
    public slots:
      void select_expt(const QString&);
      void run();
    private:
      Ami::XtcClient&  _client;
      Task*       _task;  // thread for Qt
      FileSelect* _file_select;
      QListWidget* _expt_select;
    };
  };
}

#endif
