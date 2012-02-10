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
      void insertTransition(Pds::TransitionId::Value transition);
      void printTransition(Pds::TransitionId::Value transition);
      void printDgram(const Dgram* dg);
      void getPathsFromRun(QStringList& list, QString run);
      void configure();

    public slots:
      void select_dir();
      void select_run(int);
      void run_clicked();
      void stop_clicked();
      void hzSliderChanged(int);
    private:
      void run(bool configure_only);
      void set_dir(QString dir);
      XtcClient _client;
      QString _curdir;
      Task* _task;  // thread for Qt
      QPushButton* _dir_select;
      QLabel* _dirLabel;
      QComboBox* _run_list;
      QPushButton* _runButton;
      QPushButton* _stopButton;
      QPushButton* _exitButton;
      QLabel* _startLabel;
      QLabel* _timeLabel;
      QLabel* _countLabel;
      QLabel* _payloadSizeLabel;
      QLabel* _damageLabel;
      QLabel* _hzLabel;
      QSlider* _hzSlider;
      QLabel* _hzSliderLabel;
      QCheckBox* _loopCheckBox;
      QCheckBox* _skipCheckBox;
      bool _running;
      XtcRun _run;
      bool _stopped;
      Pds::TransitionId::Value _lastTransition;
      int _dgCount;
      double _clockStart;
      double _runStart;
      uint32_t _damageMask;
      unsigned _damageCount;
      unsigned long long _payloadTotal;
    };
  };
}

#endif
