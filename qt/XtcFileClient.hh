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
#include "ami/app/XtcClient.hh"
#include "ami/service/Routine.hh"
#include "ami/service/Task.hh"

#include <errno.h>

using std::string;
using std::list;
using Pds::Dgram;

namespace Ami {

  namespace Qt {
    class XtcFileClient : public QWidget, public Routine {
      Q_OBJECT
    public:
      XtcFileClient(QGroupBox* groupBox, Ami::XtcClient& client, const char* curdir, bool testMode);
      ~XtcFileClient();
      void routine();
      void insertTransition(Pds::TransitionId::Value transition);
      void getPathsForRun(QStringList& list, QString run);
      void configure();
      void configure_run();
    public slots:
      void selectDir();
      void selectRun(int);
      void runClicked();
      void stopClicked();
      void hzSliderChanged(int);
      void printTransition(const Pds::TransitionId::Value transition);
      void printDgram(const Dgram dg);
      void setStatus(const QString s);
      void setEnabled(QWidget* widget, bool enabled);
    signals:
      void _printTransition(const Pds::TransitionId::Value transition);
      void _printDgram(const Dgram dg);
      void _setStatus(const QString s);
      void _setEnabled(QWidget* widget, bool enabled);
    private:
      void run(QString runName, bool configureOnly);
      void setDir(QString dir);
      XtcClient _client;
      QString _curdir;
      bool _testMode;
      Task* _task;  // thread for Qt
      QPushButton* _dirSelect;
      QLabel* _dirLabel;
      QComboBox* _runList;
      QString _runName;
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
      QLabel* _statusLabel;
      bool _running;
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
