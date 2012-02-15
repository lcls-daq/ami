#include <dirent.h>
#include <fcntl.h>
#include <glob.h>
#include <stdlib.h>
#include <iostream>
#include <string>

#include <QtGui/QApplication>
#include <QtGui/QFileDialog>
#include <QtGui/QVBoxLayout>
#include <QtGui/QSlider>

#include "ami/qt/XtcFileClient.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/ana/XtcRun.hh"

using namespace Ami::Qt;
using namespace Pds;
using namespace std;

const int defaultHz = 60;
const int maxHz = 240;
const double secBetweenPrintDgram = 0.2;

namespace Ami {
  namespace Qt {
    class ConfigureTask : public Routine {
    public:
      ConfigureTask(XtcFileClient& c) : _c(c) {}
      ~ConfigureTask() {}
    public:
      void routine() { _c.configure(); delete this; }
    private:
      XtcFileClient& _c;
    };
  };
};

void XtcFileClient::printTransition(const Pds::TransitionId::Value transition) {
  if (_lastTransition != transition) {
    cout << ">>> " << TransitionId::name(transition) << endl;
    _lastTransition = transition;
  }
}

static double getTimeAsDouble() {
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts); // vs CLOCK_PROCESS_CPUTIME_ID
  return ts.tv_sec + ts.tv_nsec / 1.e9;
}

static double getClockTimeAsDouble(ClockTime& clockTime) {
  return clockTime.seconds() + clockTime.nanoseconds() / 1.e9;
}

static QString itoa(int i) {
  char buf[128];
  sprintf(buf, "%d", i);
  return QString(buf);
}

void XtcFileClient::setStatus(const QString status) {
  cout << "[@@@] " << qPrintable(status) << endl;
  _statusLabel->setText(status);
}

void XtcFileClient::setEnabled(QWidget* widget, bool enabled) {
  widget->setEnabled(enabled);
}

void XtcFileClient::printDgram(const Dgram dg0) {
  const Dgram* dg = &dg0;
  char buf[256];

  double now = getTimeAsDouble();
  double deltaSec = now - _runStart;

  ClockTime clock = dg->seq.clock();
  if (_clockStart == 0.0) {
    time_t time = (time_t) clock.seconds();
    char date[256];
    strcpy(date, ctime(&time));
    date[strlen(date) - 1] = '\0';
    sprintf(buf, "Run start: %s", date);
    _startLabel->setText(buf);

    _clockStart = getClockTimeAsDouble(clock);
    sprintf(buf, "Datagram count: %d", _dgCount);
    _countLabel->setText(buf);
  } else {
    double clockDelta = getClockTimeAsDouble(clock) - _clockStart;
    sprintf(buf, "Datagram count: %d (run start + %.3fs)", _dgCount, clockDelta);
    _countLabel->setText(buf);
  }

  sprintf(buf, "Playback time: %.3fs", deltaSec);
  _timeLabel->setText(buf);

  double payloadTotalGB = _payloadTotal / 1024.0 / 1024.0 / 1024.0;
  sprintf(buf, "Total payload size: %.3f GB", payloadTotalGB);
  _payloadSizeLabel->setText(buf);

  sprintf(buf, "Damage: count = %d, mask = %x", _damageCount, _damageMask);
  _damageLabel->setText(buf);

  sprintf(buf, "Rate: %.0f Hz (%0.3f GB/s)", _dgCount / deltaSec, payloadTotalGB / deltaSec);
  _hzLabel->setText(buf);
}

void XtcFileClient::getPathsForRun(QStringList& list, QString run) {
  if (_curdir.isEmpty() || run.isEmpty()) {
    cout << "getPathsForRun: no paths to add" << endl;
    cout << "dir=" << qPrintable(_curdir) << endl;
    cout << "run=" << qPrintable(run) << endl;
    return;
  }
  cout << "_curdir=[" << qPrintable(_curdir) << "] in getPathsForRun" << endl;

  QString pattern = _curdir + "/xtc/*-r" + run + "-s*.xtc";
  glob_t g;
  cout << "getPathsForRun: " << qPrintable(pattern) << endl;
  glob(qPrintable(pattern), 0, 0, &g);

  for (unsigned i = 0; i < g.gl_pathc; i++) {
    char *path = g.gl_pathv[i];
    struct ::stat64 st;
    if (::stat64(path, &st) == -1) {
      perror(path);
      continue;
    }
    if (st.st_size == 0) {
      printf("Ignoring empty file %s\n", path);
      continue;
    }
    cout << "getPathsForRun: adding " << path << endl;
    cout << path << endl;
    list << path;
  }
  globfree(&g);

  cout << "getPathsForRun: sorting " << list.size() << " entries..." << endl;
  list.sort();
  cout << "getPathsForRun: done." << endl;
}

static void _connect(const QObject* sender, const char* signal, const QObject* receiver, const char* method, Qt::ConnectionType type = Qt::AutoConnection) {
  if (! QObject::connect(sender, signal, receiver, method, type)) {
    cout << "connect(" << sender << ", " << signal << ", " << receiver << ", " << method << ", " << type << ") failed" << endl;
    exit(1);
  }
}

XtcFileClient::XtcFileClient(QGroupBox* groupBox, Ami::XtcClient& client, const char* curdir, bool testMode) :
  QWidget (0,::Qt::Window),
  _client(client),
  _curdir(curdir),
  _testMode(testMode),
  _task(new Task(TaskObject("amiqt"))),
  _dirSelect(new QPushButton("Select")),
  _dirLabel(new QLabel(curdir)),
  _runList(new QComboBox()),
  _runName(""),

  _runButton(new QPushButton("Run")),
  _stopButton(new QPushButton("Stop")),
  _exitButton(new QPushButton("Exit")),

  _startLabel(new QLabel),
  _timeLabel(new QLabel),
  _countLabel(new QLabel),
  _payloadSizeLabel(new QLabel),
  _damageLabel(new QLabel),
  _hzLabel(new QLabel),

  _hzSlider(new QSlider(::Qt::Horizontal)),
  _hzSliderLabel(new QLabel),
  _loopCheckBox(new QCheckBox("Loop over run")),
  _skipCheckBox(new QCheckBox("Skip processing")),
  _statusLabel(new QLabel("Initializing...")),

  _running(false),
  _stopped(false),
  _lastTransition(TransitionId::Unmap),

  _damageMask(0),
  _damageCount(0)
{
  setDir(QString(_curdir));

  QVBoxLayout* l = new QVBoxLayout;

  QHBoxLayout* hboxA = new QHBoxLayout;
  hboxA->addWidget(_dirLabel);
  hboxA->addWidget(_dirSelect);
  l->addLayout(hboxA);

  l->addWidget(_runList);

  QHBoxLayout* hbox1 = new QHBoxLayout;
  hbox1->addStretch();
  hbox1->addWidget(_runButton);
  hbox1->addStretch();
  hbox1->addWidget(_stopButton);
  hbox1->addStretch();
  hbox1->addWidget(_exitButton);
  hbox1->addStretch();
  l->addLayout(hbox1);

  QFont qfont;
  qfont.setFixedPitch(true);
  _startLabel->setFont(qfont);
  _timeLabel->setFont(qfont);
  _countLabel->setFont(qfont);
  _payloadSizeLabel->setFont(qfont);
  _damageLabel->setFont(qfont);
  _hzLabel->setFont(qfont);

  l->addWidget(_startLabel);
  l->addWidget(_countLabel);
  l->addWidget(_timeLabel);
  l->addWidget(_payloadSizeLabel);
  l->addWidget(_damageLabel);
  l->addWidget(_hzLabel);

  QHBoxLayout* hbox2 = new QHBoxLayout;
  _hzSlider->setRange(1, maxHz + 1);
  _hzSlider->setValue(defaultHz);
  hbox2->addWidget(_hzSlider);
  hbox2->addWidget(_hzSliderLabel);
  hzSliderChanged(defaultHz);
  l->addLayout(hbox2);

  l->addWidget(_loopCheckBox);
  //  l->addWidget(_skipCheckBox);
  l->addWidget(_statusLabel);

  if (groupBox) {
    groupBox->setLayout(l);
  } else {
    setLayout(l);
    show();
  }

  _connect(_dirSelect, SIGNAL(clicked()), this, SLOT(selectDir()));
  _connect(_runButton, SIGNAL(clicked()), this, SLOT(runClicked()));
  _connect(_stopButton, SIGNAL(clicked()), this, SLOT(stopClicked()));
  _connect(_exitButton, SIGNAL(clicked()), qApp, SLOT(closeAllWindows()));
  _connect(_runList, SIGNAL(currentIndexChanged(int)), this, SLOT(selectRun(int)));
  _connect(_hzSlider, SIGNAL(valueChanged(int)), this, SLOT(hzSliderChanged(int)));
  _connect(this, SIGNAL(_printDgram(const Dgram)), this, SLOT(printDgram(const Dgram)));
  _connect(this, SIGNAL(_printTransition(const Pds::TransitionId::Value)), this, SLOT(printTransition(const Pds::TransitionId::Value)));
  _connect(this, SIGNAL(_setStatus(const QString)), this, SLOT(setStatus(const QString)));
  _connect(this, SIGNAL(_setEnabled(QWidget*, bool)), this, SLOT(setEnabled(QWidget*, bool)));

  configure_run();
}

void XtcFileClient::hzSliderChanged(int value) {
  char buf[128];
  if (value > maxHz) {
    sprintf(buf, "(unthrottled)");
  } else {
    sprintf(buf, "%d Hz", value);
  }
  _hzSliderLabel->setText(buf);
}

XtcFileClient::~XtcFileClient()
{
  _task->destroy();
}

static void d_sleep(double seconds) {
  long long int nanos = (long long int)(1.e9 * seconds);
  timespec sleepTime;
  const long long int billion = 1000 * 1000 * 1000;
  sleepTime.tv_sec = nanos / billion;
  sleepTime.tv_nsec = nanos % billion;
  if (nanosleep(&sleepTime, NULL) < 0) {
    perror("nanosleep");
  }
}

void XtcFileClient::selectRun(int index)
{
  _stopped = true;
  if (_running) {
    cout << "Waiting for stop..." << endl;
    while (_running) {
      d_sleep(0.2);
    }
  }
  configure_run();
}

void XtcFileClient::selectDir()
{
  cout << "selectDir called..." << endl;
  setStatus("Selecting directory...");
  setDir(QFileDialog::getExistingDirectory(0, "Select Directory", _curdir, 0));
}

void XtcFileClient::setDir(QString dir)
{
  setStatus(QString("Selected directory " + dir + "..."));
  if (dir == "") {
    return;
  }
  while (dir.endsWith("/")) {
    dir.chop(1);
  }
  if (dir.endsWith("/xtc")) {
    dir.chop(4);
  }
  _curdir.clear();
  _curdir.append(dir);
  _dirLabel->setText(dir);

  // Clear the list widget and the current run
  cout << "_curdir is " << qPrintable(_curdir) << endl;
  cout << "_runList->currentText() was " << qPrintable(_runList->currentText()) << endl;
  _runList->clear();

  QStringList runs;

  // Collect all the .xtc files in this dir,
  // and remember the runs as well.
  glob_t g;
  QString gpath = _curdir + "/xtc/*-r*-s*.xtc";
  printf("Trying %s\n", qPrintable(gpath));
  glob(qPrintable(gpath), 0, 0, &g);
  for (unsigned i = 0; i < g.gl_pathc; i++) {
    char *path = g.gl_pathv[i];
    QString s(basename(path));
    int p = s.indexOf("-r");
    if (p >= 0) {
      QString run = s.mid(p+2, s.indexOf("-s")-p-2);
      if (! runs.contains(run)) {
        runs << run;
      }
    }
  }
  globfree(&g);
  runs.sort();
  _runList->addItems(runs);

  setStatus(QString("Found ") + itoa(runs.size()) + " runs");
  printf("Found %d runs\n", runs.size());
}

void XtcFileClient::runClicked() 
{
  setStatus("Run requested...");
  _runButton->setEnabled(false);
  _runName = _runList->currentText();
  _task->call(this);
}

void XtcFileClient::stopClicked() {
  setStatus("Stop requested...");
  _stopped = true;
}

void XtcFileClient::insertTransition(Pds::TransitionId::Value transition)
{
  printTransition(transition);
  Dgram dg;
  new((void*)&dg.seq) Pds::Sequence(Pds::Sequence::Event, transition, Pds::ClockTime(0,0), Pds::TimeStamp(0,0,0,0));
  new((char*)&dg.xtc) Pds::Xtc(Pds::TypeId(Pds::TypeId::Id_Xtc,0),Pds::ProcInfo(Pds::Level::Recorder,0,0));
  _client.processDgram(&dg);
}

void XtcFileClient::routine()
{
  emit _setStatus("Running run " + _runName + "...");
  emit _setEnabled(_stopButton, true);
  emit _setEnabled(_runButton, false);
  _running = true;
  _stopped = false;

  do {
    run(_runName, false);
  } while (! _stopped && _loopCheckBox->isChecked());

  _stopped = false;
  _running = false;
  emit _setEnabled(_stopButton, false);
  emit _setEnabled(_runButton, true);
  emit _setStatus("Stopped run " + _runName + ".");
}

void XtcFileClient::configure_run()
{
  _runName = _runList->currentText();
  _task->call(new ConfigureTask(*this));
}

void XtcFileClient::configure()
{
  if (_testMode) {
    _runList->setCurrentIndex(0); // XXX no Qt operations should be done in this thread!
    for (int i = 1; true; i++) {
      _runName = _runList->currentText();
      _stopped = false;
      _running = false;
      run(_runName, true); // true to do full run; false to just config
      _runList->setCurrentIndex(_runList->currentIndex() + 1);
      cout << "~~~~~ did configure #" << i << endl;
    }
  } else {
    if (_runName == "") {
      emit _setStatus("No runs found.");
      return;
    }
    emit _setStatus("Configuring run " + _runName + "...");
    run(_runName, true);
    emit _setStatus("Configured run " + _runName + ".");
  }
}

void XtcFileClient::run(QString runName, bool configureOnly)
{
  QStringList files;
  cout << "_curdir is " << qPrintable(_curdir) << endl;
  getPathsForRun(files, runName);
  if (files.empty()) {
    cout << "getPathsForRun(): No xtc files found in " << qPrintable(_curdir) << " for run " << qPrintable(runName) << endl;
    _stopped = true; // do not loop
    return;
  }

  Pds::Ana::XtcRun run;
  run.live_read(false); // These files are already completely written
  string file = qPrintable(files.first());
  run.reset(file);
  files.pop_front();
  while (! files.empty()) {
    file = qPrintable(files.first());
    run.add_file(file);
    files.pop_front();
  }
  _setStatus("Initializing run " + runName + "...");
  run.init();
  _setStatus("Initialized run " + runName + "...");

  _runStart = getTimeAsDouble();
  _clockStart = 0.0;
  _dgCount = 0;
  _damageMask = 0;
  _damageCount = 0;
  _payloadTotal = 0;

  insertTransition(TransitionId::Map);

  double lastPrintTime = 0.0;

  while (configureOnly || ! _stopped) {
    Dgram* dg = NULL;
    int slice = -1;
    int64_t offset = -1;
    Pds::Ana::Result result = run.next(dg, &slice, &offset);
    if (result != Pds::Ana::OK) {
      break;
    }
    Pds::TransitionId::Value transition = dg->seq.service();

    printTransition(transition);

    _dgCount++;
    _payloadTotal += dg->xtc.sizeofPayload();

    if (transition != TransitionId::L1Accept) {
      _client.processDgram(dg);
    } else if (! _skipCheckBox->isChecked()) {
      _client.processDgram(dg);
    }

    uint32_t damage = dg->xtc.damage.value();
    if (damage) {
        _damageCount++;
        _damageMask |= damage;
    }

    if (transition == TransitionId::L1Accept) {
      double now = getTimeAsDouble();
      if (lastPrintTime < now - secBetweenPrintDgram) {
        emit _printDgram(*dg);
        lastPrintTime = now;
      }

      int hz = _hzSlider->value();
      if (hz <= maxHz) {
        double deltaSec = now - _runStart;
        double desiredDeltaSec = _dgCount / (double) hz;
        //cout << "hz=" << hz << " desiredDeltaSec = " << desiredDeltaSec << endl;
        double stallSec = desiredDeltaSec - deltaSec;
        if (stallSec > 2.0) {
          stallSec = 2.0;
        }
        if (stallSec > 0.0) {
          //cout << "Need to stall for " << stallSec << " seconds." << endl;
          d_sleep(stallSec);
        }
      }
    } else if (transition == TransitionId::Configure && configureOnly) {
      break;
    }
  }

  insertTransition(TransitionId::Unconfigure);
  insertTransition(TransitionId::Unmap);
}
