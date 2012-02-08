#ifndef AmiQt_XtcFileClient_hh
#define AmiQt_XtcFileClient_hh

#include <QtGui/QComboBox>
#include <QtGui/QGroupBox>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QWidget>
#include <QtGui/QCheckBox>
#include <QtGui/QSpinBox>

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/ana/XtcRun.hh"
#include "ami/app/XtcClient.hh"
#include "ami/service/Routine.hh"
#include "ami/service/Task.hh"

#include <errno.h>

using std::string;
using std::list;
using Pds::Dgram;
using Pds::Ana::XtcRun;

namespace Ami {

  namespace Qt {
    class XtcFileClient : public QWidget, public Routine {
      Q_OBJECT
    public:
      XtcFileClient(QGroupBox* groupBox, Ami::XtcClient& client, const char* curdir);
      ~XtcFileClient();
      void routine();
      void NEW_routine();
      void insertTransition(Pds::TransitionId::Value transition);
      void printTransition(const Dgram* dg, double& start, const double effectiveHz);
      void getPathsFromRun(QStringList& list, QString run);
      void configure();

    public slots:
      void select_dir();
      void select_run();
      void run();
      void stop();
    private:
      XtcClient _client;
      QString _curdir;
      Dgram* _dg;
      unsigned _interface;
      unsigned _serverGroup;
      Task* _task;  // thread for Qt
      QPushButton* _dir_select;
      QLabel* _dirLabel;
      QComboBox* _run_list;
      QPushButton* _runButton;
      QPushButton* _stopButton;
      QPushButton* _exitButton;
      QLabel* _transitionLabel;
      QLabel* _clockLabel;
      QLabel* _timeLabel;
      QLabel* _payloadSizeLabel;
      QLabel* _damageLabel;
      QLabel* _hzLabel;
      QSpinBox* _hzSpinBox;
      QCheckBox* _loopCheckBox;
      bool _running;
      list<string> _paths;
      XtcRun _run;
      bool _runIsValid;
      bool _stopped;
      int _hz;
      bool _verbose;
      bool _veryverbose;
      bool _skipToNextRun();
      void _addPaths(list<string> newPaths);
      void set_dir(QString dir);
      long long int timeDiff(struct timespec* end, struct timespec* start);
      Dgram* next();
    };
  };
}

#endif
