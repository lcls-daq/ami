#include "ami/qt/XtcFileClient.hh"

#include <dirent.h>
#include <fcntl.h>
#include <glob.h>
#include <stdlib.h>
#include <iostream>
#include <string>

const int maxHz = 60;
const int defaultHz = maxHz + 1;
const double secBetweenPrintDgram = 0.2;
const bool testModeConfigOnly = false;

static pthread_t _qtThread;

using namespace Ami::Qt;

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

static QString asctime(time_t& t) {
  char buf[128];
  strcpy(buf, asctime(localtime(&t)));
  buf[strlen(buf) - 1] = '\0';
  return QString(buf);
}

void XtcFileClient::setStatusLabelText(const QString status) {
  _statusLabel->setText(status);
}

void XtcFileClient::setStatus(const QString status) {
  cout << qPrintable(status) << endl;
  if (pthread_equal(pthread_self(), _qtThread)) {
    setStatusLabelText(status);
  } else {
    emit _setStatusLabelText(status);
  }
}

void XtcFileClient::updateDirLabel() {
  _dirLabel->setText("Folder: " + _curdir);
}

void XtcFileClient::updateRunCombo() {
  _runCombo->clear();
  _runCombo->addItems(_runList);
}

void XtcFileClient::updateRun() {
  int index = _runCombo->findText(_runName);
  if (index != -1) {
    _runCombo->setCurrentIndex(index);
  }
}

void XtcFileClient::setEnabled(QWidget* widget, bool enabled) {
  widget->setEnabled(enabled);
}

void XtcFileClient::printDgram(const Dgram dg0) {
  const Dgram* dg = &dg0;

  if (dg->seq.service() == TransitionId::Configure) {
    time_t t = (time_t) _runStart;
    _startLabel->setText("Run start: " + asctime(t));
    return;
  }

  double now = getTimeAsDouble();
  double deltaSec = now - _executionStart;
  char buf[256];

  ClockTime clockTime = dg->seq.clock();
  double clockDelta = getClockTimeAsDouble(clockTime) - _runStart;
  sprintf(buf, "Datagram count: %d (run start + %.3fs)", _dgCount, clockDelta);
  _countLabel->setText(buf);

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
    setStatus("getPathsForRun: no paths to add for directory " + _curdir + " run " + run);
    return;
  }

  QString pattern = _curdir + "/xtc/*-r" + run + "-s*.xtc";
  glob_t g;
  glob(qPrintable(pattern), 0, 0, &g);

  for (unsigned i = 0; i < g.gl_pathc; i++) {
    char *path = g.gl_pathv[i];
    struct ::stat64 st;
    if (::stat64(path, &st) == -1) {
      perror(path);
      continue;
    }
    if (st.st_size == 0) {
      setStatus(QString("Ignoring empty file ") + path);
      continue;
    }
    list << path;
  }
  globfree(&g);
  list.sort();
}

static void _connect(const QObject* sender, const char* signal, const QObject* receiver, const char* method, ::Qt::ConnectionType type = ::Qt::AutoConnection) {
  if (! QObject::connect(sender, signal, receiver, method, type)) {
    cerr << "connect(" << sender << ", " << signal << ", " << receiver << ", " << method << ", " << type << ") failed" << endl;
    exit(1);
  }
}

XtcFileClient::XtcFileClient(QGroupBox* groupBox, XtcClient& client, const char* curdir, bool testMode) :
  QWidget (0,::Qt::Window),
  _runValid(false),
  _client(client),
  _curdir(curdir),
  _testMode(testMode),
  _task(new Task(TaskObject("amiqt"))),
  _dirSelect(new QPushButton("Change")),
  _dirLabel(new QLabel),
  _runCombo(new QComboBox),
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
  _statusLabel(new QLabel("Initializing...")),

  _running(false),
  _stopped(false),

  _damageMask(0),
  _damageCount(0)
{
  _qtThread = pthread_self();

  QVBoxLayout* l = new QVBoxLayout;

  QHBoxLayout* hboxA = new QHBoxLayout;
  hboxA->addWidget(_dirLabel);
  hboxA->addWidget(_dirSelect);
  l->addLayout(hboxA);
  updateDirLabel();

  QHBoxLayout* hbox1 = new QHBoxLayout;
  hbox1->addStretch();
  hbox1->addWidget(new QLabel("Run:"));
  hbox1->addStretch();
  hbox1->addWidget(_runCombo);
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
  hbox2->addWidget(new QLabel("Throttle:"));
  hbox2->addWidget(_hzSlider);
  hbox2->addWidget(_hzSliderLabel);
  hzSliderChanged(defaultHz);
  l->addLayout(hbox2);

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
  _connect(_runCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(selectRun(int)));
  _connect(_hzSlider, SIGNAL(valueChanged(int)), this, SLOT(hzSliderChanged(int)));
  _connect(this, SIGNAL(_printDgram(const Dgram)), this, SLOT(printDgram(const Dgram)));
  _connect(this, SIGNAL(_setStatusLabelText(const QString)), this, SLOT(setStatusLabelText(const QString)));
  _connect(this, SIGNAL(_setEnabled(QWidget*, bool)), this, SLOT(setEnabled(QWidget*, bool)));
  _connect(this, SIGNAL(_updateDirLabel()), this, SLOT(updateDirLabel()));
  _connect(this, SIGNAL(_updateRunCombo()), this, SLOT(updateRunCombo()));
  _connect(this, SIGNAL(_updateRun()), this, SLOT(updateRun()));

  setDir(QString(_curdir));
  configure_run();
}

void XtcFileClient::hzSliderChanged(int value) {
  if (value > maxHz) {
    _hzSliderLabel->setText("(none)");
  } else {
    _hzSliderLabel->setText(itoa(value) + " Hz");
  }
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
  if (_running) {
    setStatus("Waiting for stop...");
    while (_running) {
      d_sleep(0.2);
    }
  }
  configure_run();
}

void XtcFileClient::selectDir()
{
  setStatus("Selecting directory...");
  setDir(QFileDialog::getExistingDirectory(0, "Select Directory", _curdir, 0));
}

void XtcFileClient::setDir(QString dir)
{
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
  emit _updateDirLabel();


  // Collect all the runs in this dir.
  _runList.clear();
  _runName = "";
  glob_t g;
  QString gpath = _curdir + "/xtc/*-r*-s*.xtc";
  setStatus("Looking for runs under " + gpath + "...");
  glob(qPrintable(gpath), 0, 0, &g);
  for (unsigned i = 0; i < g.gl_pathc; i++) {
    char *path = g.gl_pathv[i];
    QString s(basename(path));
    int p = s.indexOf("-r");
    if (p >= 0) {
      QString run = s.mid(p+2, s.indexOf("-s")-p-2);
      if (! _runList.contains(run)) {
        _runList << run;
        if (_runName == "") {
          _runName = run;
        }
      }
    }
  }
  globfree(&g);
  _runList.sort();
  setStatus(QString("Found ") + itoa(_runList.size()) + " runs;;; runName=" + _runName);
  emit _updateRunCombo();
  if (_runName != "") {
    emit _updateRun();
  }
}

void XtcFileClient::runClicked() 
{
  setStatus("Run requested...");
  _runButton->setEnabled(false);
  _runName = _runCombo->currentText();
  _task->call(this);
}

void XtcFileClient::stopClicked() {
  setStatus("Stop requested...");
  _stopped = true;
}

void XtcFileClient::insertTransition(TransitionId::Value transition)
{
  Dgram dg;
  new((void*)&dg.seq) Sequence(Sequence::Event, transition, ClockTime(0,0), TimeStamp(0,0,0,0));
  new((char*)&dg.xtc) Xtc(TypeId(TypeId::Id_Xtc,0),ProcInfo(Level::Recorder,0,0));
  _client.processDgram(&dg);
}

void XtcFileClient::routine()
{
  emit _setEnabled(_stopButton, true);
  emit _setEnabled(_runButton, false);
  _running = true;
  _stopped = false;

  do_configure(_runName);
  if (_runValid) {
    run();
  }

  _stopped = false;
  _running = false;
  emit _setEnabled(_stopButton, false);
  emit _setEnabled(_runButton, true);
}

void XtcFileClient::configure_run()
{
  _runName = _runCombo->currentText();
  _task->call(new ConfigureTask(*this));
}

void XtcFileClient::configure()
{
  if (_testMode) {
    // _curdir either /reg/d or /reg/data or /reg/data/ana01 or /reg/data/ana01/cxi or...
    QStringList list;
    QString endPattern = "/xtc";
    for (int depth = 1; depth <= 6; depth++) {
      QString pattern = _curdir + endPattern;
      setStatus("Looking for runs under " + pattern + "...");
      glob_t g;
      glob(qPrintable(pattern), 0, 0, &g);
      for (unsigned i = 0; i < g.gl_pathc; i++) {
        char *path = g.gl_pathv[i];
        list << path;
      }
      globfree(&g);
      setStatus("Found " + itoa(list.size()) + " runs under " + pattern + ".");
      if (list.size() > 1) { // XXX
        break;
      }
      endPattern = "/*" + endPattern;
    }
    if (list.size() == 0) {
      setStatus("No runs found under " + _curdir);
      exit(1);
    }
    list.sort();
    int runCount = 0;
    int dgCount = 0;
    for (int i = 0; i < list.size(); i++) {
      QString dir = list.at(i);
      setDir(dir);
      for (int j = 0; j < _runList.size(); j++) {
        _runName = _runList.at(j);
        emit _updateRun();
        _stopped = false;
        _running = false;
        do_configure(_runName);
        if (! _runValid) {
          setStatus("Configuration for run " + _runName + " failed");
        } else if (! testModeConfigOnly) {
          run();
        }
        runCount++;
        dgCount += _dgCount;
        time_t t;
        time(&t);
        printf("----------[ %s - %d runs, %d datagrams ]------------------------------\n",
               qPrintable(asctime(t)), runCount, dgCount);
      }
    }
    printf("----------[ ALL DONE ]------------------------------\n");
    exit(0);
  } else {
    if (_runName == "") {
      setStatus("No runs found.");
      return;
    }
    do_configure(_runName);
  }
}

void XtcFileClient::do_configure(QString runName)
{
  _runValid = false;
  QStringList files;
  setStatus("Fetching files for run " + runName);
  getPathsForRun(files, runName);
  if (files.empty()) {
    setStatus("Found no files paths for run " + runName);
    return;
  }
  setStatus("Fetched " + itoa(files.size()) + " paths for run " + runName);

  _run.live_read(false); // These files are already completely written
  string file = qPrintable(files.first());
  _run.reset(file);
  files.pop_front();
  while (! files.empty()) {
    file = qPrintable(files.first());
    _run.add_file(file);
    files.pop_front();
  }

  setStatus("Initializing run " + runName);
  _run.init();

  setStatus("Fetching first datagram for run " + runName);
  _dgCount = 0;
  insertTransition(TransitionId::Map);

  Dgram* dg = NULL;
  int slice = -1;
  int64_t offset = -1;
  Result result = _run.next(dg, &slice, &offset);
  if (result != OK) {
    setStatus("Could not fetch Configuration datagram");
    return;
  }
  if (dg->seq.service() != TransitionId::Configure) {
    setStatus("Run does not start with Configuration datagram");
    return;
  }

  _dgCount++;
  ClockTime clockTime = dg->seq.clock();
  _executionStart = getTimeAsDouble();
  _runStart = getClockTimeAsDouble(clockTime);
  _damageMask = 0;
  _damageCount = 0;
  _payloadTotal = 0;

  emit _printDgram(*dg);

  setStatus("Configuring run " + runName);
  _client.processDgram(dg);
  setStatus("Finished configuring run " + runName + ".");
  _runValid = true;

  //  _run.probe();
}

void XtcFileClient::run()
{
  _executionStart = getTimeAsDouble();
  _damageMask = 0;
  _damageCount = 0;
  _payloadTotal = 0;

  double lastPrintTime = 0.0;

  Dgram* last_dg = NULL;
  Dgram* last_printed_dg = NULL;

  while (! _stopped) {
    Dgram* dg = NULL;
    int slice = -1;
    int64_t offset = -1;
    Result result = _run.next(dg, &slice, &offset);
    if (result != OK) {
      break;
    }
    last_dg = dg;
    _dgCount++;
    _payloadTotal += dg->xtc.sizeofPayload();
    _client.processDgram(dg);

    uint32_t damage = dg->xtc.damage.value();
    if (damage) {
        _damageCount++;
        _damageMask |= damage;
    }

    TransitionId::Value transition = dg->seq.service();
    if (transition == TransitionId::Configure) {
      setStatus("Configuring run " + _runName + " ???");
      _client.processDgram(dg);
      setStatus("Finished configuring run " + _runName + " ???");
      sleep(5);
      continue;
    }

    if (transition == TransitionId::L1Accept) {
      double now = getTimeAsDouble();
      if (lastPrintTime < now - secBetweenPrintDgram) {
        emit _printDgram(*dg);
        lastPrintTime = now;
        last_printed_dg = dg;
      }

      int hz = _hzSlider->value();
      if (hz <= maxHz) {
        double deltaSec = now - _executionStart;
        double desiredDeltaSec = _dgCount / (double) hz;
        double stallSec = desiredDeltaSec - deltaSec;
        if (stallSec > 2.0) {
          stallSec = 2.0;
        }
        if (stallSec > 0.0) {
          d_sleep(stallSec);
        }
      }
    }
  }
  if (last_dg != NULL && last_dg != last_printed_dg) {
    emit _printDgram(*last_dg);
  }
  QString verb = (_stopped ? "Stopped" : "Completed");
  setStatus(verb + " run " + _runName + " (" + itoa(_dgCount) + " datagrams)");

  insertTransition(TransitionId::Unconfigure);
  insertTransition(TransitionId::Unmap);
}
