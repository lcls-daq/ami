#ifndef AmiQt_XtcFileClient_hh
#define AmiQt_XtcFileClient_hh

#include <QtGui/QComboBox>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QWidget>
#include <QtGui/QCheckBox>

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/ana/XtcRun.hh"
#include "ami/qt/QtMonitorClient.hh"

#include "ami/service/Routine.hh"
#include "ami/service/Task.hh"
#include "ami/qt/QtMonitorClient.hh"

class QtMonitorServer;

using std::string;
using std::list;
using Pds::Dgram;
using Pds::Ana::XtcRun;

namespace Ami {

  namespace Qt {
    class XtcFileClient : public QWidget, public Routine, public QtMonitorClient {
      Q_OBJECT
    public:
      XtcFileClient(const char* partitionTag, unsigned interface, unsigned serverGroup, const char* basedir);
      ~XtcFileClient();
      void routine();
      void printTransition(const Dgram* dg, const double hz = 0);
      void addPathsFromRun(QString run);
      void loopCheckBoxChanged(int state);

    public slots:
      void select_dir();
      void run();
      void stop();
      void ready();
    signals:
      void done();
    private:
      QString _curdir;
      unsigned _interface;
      unsigned _serverGroup;
      Task* _task;  // thread for Qt
      QPushButton* _dir_select;
      QLabel* _dirLabel;
      QComboBox* _file_select;
      QPushButton* _runButton;
      QPushButton* _stopButton;
      QPushButton* _exitButton;
      QLabel* _transitionLabel;
      QLabel* _timeLabel;
      QLabel* _payloadSizeLabel;
      QLabel* _damageLabel;
      QLabel* _hzLabel;
      //QSlider* _hzSlider;
      QCheckBox* _loopCheckBox;
      bool _running;




      list<string> _paths;
      XtcRun _run;
      QtMonitorServer* _server;
      bool _runIsValid;
      bool _stopped;
      long long int _period;
      bool _verbose;
      bool _veryverbose;
      bool _skipToNextRun();
      void _addPaths(list<string> newPaths);
      long long int timeDiff(struct timespec* end, struct timespec* start);
      Dgram* next();
    };
  };
}

#endif
