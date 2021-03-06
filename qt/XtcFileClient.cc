#include "ami/qt/XtcFileClient.hh"

#include "ami/event/Calib.hh"

#include <QtCore/QDateTime>

#include <dirent.h>
#include <fcntl.h>
#include <glob.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <iostream>
#include <string>
#include <cassert>

const int maxHz = 60;
const int defaultHz = maxHz + 1;
const double secBetweenPrintDgram = 0.5;
const bool testModeConfigOnly = false;
const int maxTicks = 20;

static pthread_t _qtThread;

using namespace Ami::Qt;

namespace Ami {
  namespace Qt {
    class ConfigureTask : public Routine {
    public:
      ConfigureTask(XtcFileClient& c) : _c(c) {}
      ~ConfigureTask() {}
    public:
      void routine() {
        _c.configure();
        delete this;
      }
    private:
      XtcFileClient& _c;
    };
  };
};

static double now() {
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts); // vs CLOCK_PROCESS_CPUTIME_ID
  return ClockTime(ts).asDouble();
}

static QString itoa(int i) {
  char buf[128];
  sprintf(buf, "%d", i);
  return QString(buf);
}

static QString dtoa(double d, int digits) {
  char buf[128];
  sprintf(buf, "%.*f", digits, d);
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

void XtcFileClient::setPauseButtonLabel(const QString label) {
  _pauseButton->setText(label);
}

void XtcFileClient::setStatus(const QString status) {
  cout << "XtcFileClient: " << qPrintable(status) << endl;
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

void XtcFileClient::runSliderSetRange(double start, double end) {
  _runSliderBeingSet = true;
  int max = (int) (end - start);
  _runSlider->setRange(0, max);
  _runSliderMovedTo = -1;
  _runSliderBeingSet = false;
}

static int getTickInterval(double length) {
  for (int base = 1; true; base *= 10) {
    int interval = base;
    if (length / interval < maxTicks) {
      return interval;
    }
    interval = 2 * base;
    if (length / interval < maxTicks) {
      return interval;
    }
    interval = 5 * base;
    if (length / interval < maxTicks) {
      return interval;
    }
  }
}

void XtcFileClient::printDgram(const Dgram dg) {
  char buf[256];

  if (dg.seq.service() == TransitionId::Configure) {
    if (_length > 0) {
      runSliderSetRange(_start, _end);
      int interval = getTickInterval(_length);
      _runSlider->setTickInterval(interval);
      sprintf(buf, "%d sec/tick", interval);
      _runSliderLabel->setText(buf);
    } else {
      runSliderSetRange(0, 0);
      _runSliderLabel->setText("");
    }

    time_t t = (time_t) _start;
    QString label = "Run " + _runName + " date: " + asctime(t);
    if (_length > 0) {
      if (_length > 2 * 60) {
        label += " duration: " + dtoa(_length / 60, 1) + " min";
      } else {
        label += " duration: " + dtoa(_length, 1) + " sec";
      }
    }

    int numTotalEvent = 0, numCalib = 0;
    _run.numTotalEvent(numTotalEvent);
    _run.numCalib     (numCalib);
    label += "\nTotal event# " + dtoa(numTotalEvent,0) + " calib# " + dtoa(numCalib,0);

    _startLabel->setText(label);
    _eventLabel->setText("");
    _damageLabel->setText("");
    _hzLabel->setText("");

    return;
  }

  double clockTime = dg.seq.clock().asDouble();
  if (_length > 0 && (now() - _runSliderLastMoved > 0.5)) {
    _runSliderBeingSet = true;
    _runSlider->setValue((int) (clockTime - _start));
    _runSliderBeingSet = false;
  }

  int iEventGlobal, iCalib, iEventInCalib;
  _run.curEventId( iEventGlobal, iCalib, iEventInCalib );
  double clockDelta = clockTime - _start;
  int minDelta   = 0;
  if (clockDelta >= 60)
  {
    minDelta = int(clockDelta/60);
    clockDelta = clockDelta - minDelta * 60;
    sprintf(buf, "Current calib# %d event %d (global %d); Progress: %d min %.1f sec",
            iCalib, iEventInCalib, iEventGlobal, minDelta, clockDelta);
  }
  else
    sprintf(buf, "Current calib# %d event %d (global %d); Progress: %.1f sec", iCalib, iEventInCalib, iEventGlobal, clockDelta);

  QDateTime datime; datime.setTime_t(dg.seq.clock().seconds());
  QString eventLabel = QString("%1\n\t%2.%3/0x%4")
    .arg(buf)
    .arg(datime.toString("MMM dd,yyyy hh:mm:ss"))
    .arg(QString::number(dg.seq.clock().nanoseconds()),9,QChar('0'))
    .arg(QString::number(dg.seq.stamp().fiducials(),16),5,QChar('0'));
  _eventLabel->setText(eventLabel);

  sprintf(buf, "Damage: count = %d, mask = %x", _damageCount, _damageMask);
  _damageLabel->setText(buf);

  double executionTime = now() - _executionStart;
  double payloadTotalGB = _payloadTotal / 1024.0 / 1024.0 / 1024.0;
  sprintf(buf, "Rate: %.0f Hz (%0.3f GB/s)", _dgCount / executionTime, payloadTotalGB / executionTime);
  _hzLabel->setText(buf);
}

void XtcFileClient::getPathsForRun(QStringList& list, QString run) {
  if (_curdir.isEmpty() || run.isEmpty()) {
    setStatus("getPathsForRun: no paths to add for directory " + _curdir + " run " + run);
    return;
  }

  QString pattern = _curdir + "/xtc/*-r" + run + "-s*.xtc*";
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

XtcFileClient::XtcFileClient(QGroupBox* groupBox, std::vector<XtcClientT*>& client, const char* curdir, bool testMode, bool liveReadMode) :
  QWidget (0,::Qt::Window),
  _runValid(false),
  _client(client),
  _iclient(0),
  _curdir(curdir),
  _testMode(testMode),
  _liveReadMode(liveReadMode),
  _task(new Task(TaskObject("amiqt"))),
  _dirSelect(new QPushButton("Change")),
  _dirLabel(new QLabel),
  _runCombo(new QComboBox),
  _runName(""),

  _updateButton(new QPushButton("Update")),
  _runButton(new QPushButton("Run")),
  _pauseButton(new QPushButton("Pause")),
  _stopButton(new QPushButton("Stop")),
  _exitButton(new QPushButton("Exit")),

  _prevCalibButton(new QPushButton("Prev Calib")),
  _prevEventButton(new QPushButton("Prev Event")),
  _nextEventButton(new QPushButton("Next Event")),
  _nextCalibButton(new QPushButton("Next Calib")),

  _jumpLabel1(new QLabel("Jump to Calib:")),
  _jumpCalibInput(new QLineEdit),
  _jumpLabel2(new QLabel("Event:")),
  _jumpEventInput(new QLineEdit),
  _jumpLabel3(new QLabel("OR Time: Min")),
  _jumpMinInput(new QLineEdit),
  _jumpLabel4(new QLabel("Sec")),
  _jumpSecInput(new QLineEdit),

  _startLabel(new QLabel),
  _eventLabel(new QLabel),
  _damageLabel(new QLabel),

  _runSlider(new QSlider(::Qt::Horizontal)),
  _runSliderLabel(new QLabel),
  _runSliderLastMoved(0.0),
  _runSliderMovedTo(-1),
  _runSliderBeingSet(false),

  _iJumpCalib(-1),
  _iJumpEvent(-1),
  _fJumpMin(-1),
  _fJumpSec(-1),
  _iJumpToNextCalib(0),
  _iJumpToNextEvent(0),

  _hzLabel(new QLabel),
  _hzSlider(new QSlider(::Qt::Horizontal)),
  _hzSliderLabel(new QLabel),

  _statusLabel(new QLabel("Initializing...")),

  _running(false),
  _paused (false),
  _stopped(false),
  _needs_configure(false),

  _damageMask(0),
  _damageCount(0),

  _start(0),
  _end(0),
  _length(0)
{
  _qtThread = pthread_self();
  QVBoxLayout* l = new QVBoxLayout;

  QHBoxLayout* hboxA = new QHBoxLayout;
  hboxA->addWidget(_dirLabel);
  hboxA->addWidget(_dirSelect);
  hboxA->addWidget(_updateButton);
  l->addLayout(hboxA);
  updateDirLabel();

  QHBoxLayout* hboxRun = new QHBoxLayout;
  _runSliderBeingSet = true;
  runSliderSetRange(0, 0);
  _runSlider->setTickInterval(10);
  _runSlider->setTickPosition(QSlider::TicksBothSides);
  hboxRun->addWidget(new QLabel("Progress:"));
  hboxRun->addWidget(_runSlider);
  hboxRun->addWidget(_runSliderLabel);
  _runSliderBeingSet = false;
  l->addLayout(hboxRun);

  // This forces combobox to use scrollbar, which is essential
  // when it contains a very large number of items.
  _runCombo->setStyle(new QPlastiqueStyle());

  QHBoxLayout* hbox1 = new QHBoxLayout;
  hbox1->addStretch();
  hbox1->addWidget(new QLabel("Run:"));
  hbox1->addStretch();
  hbox1->addWidget(_runCombo);
  hbox1->addStretch();
  hbox1->addWidget(_runButton);
  hbox1->addStretch();
  hbox1->addWidget(_pauseButton);
  hbox1->addStretch();
  hbox1->addWidget(_stopButton);
  hbox1->addStretch();
  hbox1->addWidget(_exitButton);
  hbox1->addStretch();
  l->addLayout(hbox1);

  QHBoxLayout* hboxNextEvent = new QHBoxLayout;
  hboxNextEvent->addStretch();
  hboxNextEvent->addWidget(_prevCalibButton);
  hboxNextEvent->addStretch();
  hboxNextEvent->addWidget(_prevEventButton);
  hboxNextEvent->addStretch();
  hboxNextEvent->addWidget(_nextEventButton);
  hboxNextEvent->addStretch();
  hboxNextEvent->addWidget(_nextCalibButton);
  hboxNextEvent->addStretch();
  l->addLayout(hboxNextEvent);

  QHBoxLayout* hboxjump = new QHBoxLayout;
  hboxjump->addWidget(_jumpLabel1);
  hboxjump->addStretch();
  hboxjump->addWidget(_jumpCalibInput);
  hboxjump->addStretch();
  hboxjump->addWidget(_jumpLabel2);
  hboxjump->addStretch();
  hboxjump->addWidget(_jumpEventInput);
  hboxjump->addStretch();
  hboxjump->addWidget(_jumpLabel3);
  hboxjump->addStretch();
  hboxjump->addWidget(_jumpMinInput);
  hboxjump->addStretch();
  hboxjump->addWidget(_jumpLabel4);
  hboxjump->addStretch();
  hboxjump->addWidget(_jumpSecInput);
  hboxjump->addStretch();
  l->addLayout(hboxjump);

  l->addWidget(_startLabel);
  l->addWidget(_eventLabel);
  l->addWidget(_damageLabel);
  l->addWidget(_hzLabel);

  QHBoxLayout* hbox2 = new QHBoxLayout;
  _hzSlider->setMinimum(1);
  _hzSlider->setMaximum(maxHz + 1);
  _hzSlider->setValue(defaultHz);
  _hzSlider->setTickPosition(QSlider::TicksBothSides);
  _hzSlider->setTickInterval(10);
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
  _connect(_updateButton, SIGNAL(clicked()), this, SLOT(updateDir()));
  _connect(_runButton, SIGNAL(clicked()), this, SLOT(runClicked()));
  _connect(_pauseButton, SIGNAL(clicked()), this, SLOT(pauseClicked()));
  _connect(_stopButton, SIGNAL(clicked()), this, SLOT(stopClicked()));
  _connect(_exitButton, SIGNAL(clicked()), qApp, SLOT(closeAllWindows()));

  _connect(_prevCalibButton, SIGNAL(clicked()), this, SLOT(prevCalibClicked()));
  _connect(_prevEventButton, SIGNAL(clicked()), this, SLOT(prevEventClicked()));
  _connect(_nextEventButton, SIGNAL(clicked()), this, SLOT(nextEventClicked()));
  _connect(_nextCalibButton, SIGNAL(clicked()), this, SLOT(nextCalibClicked()));

  _connect(_jumpCalibInput, SIGNAL(returnPressed()), this, SLOT(jumpToCalibAndEvent()));
  _connect(_jumpEventInput, SIGNAL(returnPressed()), this, SLOT(jumpToCalibAndEvent()));
  _connect(_jumpMinInput, SIGNAL(returnPressed()), this, SLOT(jumpToTime()));
  _connect(_jumpSecInput, SIGNAL(returnPressed()), this, SLOT(jumpToTime()));

  _connect(_runCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(selectRun(int)));
  _connect(_hzSlider, SIGNAL(valueChanged(int)), this, SLOT(hzSliderChanged(int)));
  _connect(_runSlider, SIGNAL(valueChanged(int)), this, SLOT(runSliderChanged(int)));
  _connect(this, SIGNAL(_printDgram(const Dgram)), this, SLOT(printDgram(const Dgram)));
  _connect(this, SIGNAL(_setStatusLabelText(const QString)), this, SLOT(setStatusLabelText(const QString)));
  _connect(this, SIGNAL(_setPauseButtonLabel(const QString)), this, SLOT(setPauseButtonLabel(const QString)));
  _connect(this, SIGNAL(_setEnabled(QWidget*, bool)), this, SLOT(setEnabled(QWidget*, bool)));
  _connect(this, SIGNAL(_updateDirLabel()), this, SLOT(updateDirLabel()));
  _connect(this, SIGNAL(_updateRunCombo()), this, SLOT(updateRunCombo()));
  _connect(this, SIGNAL(_updateRun()), this, SLOT(updateRun()));

  emit _setEnabled(_pauseButton, false);
  emit _setEnabled(_stopButton, false);
  emit _setEnabled(_prevCalibButton, false);
  emit _setEnabled(_prevEventButton, false);
  emit _setEnabled(_nextEventButton, false);
  emit _setEnabled(_nextCalibButton, false);

  setDir(QString(_curdir));
  //  configure_run();
}

void XtcFileClient::hzSliderChanged(int value) {
  if (value > maxHz) {
    _hzSliderLabel->setText("(none)");
  } else {
    _hzSliderLabel->setText(itoa(value) + " Hz");
  }
  _hzSliderValue = value;

  // Reset values, but avoid values that would lead to division by zero when computing rate.
  _executionStart = now() - 1.0;
  _dgCount = 1;
}

void XtcFileClient::runSliderChanged(int value) {
  if (! _runSliderBeingSet) {
    _runSliderMovedTo = value;
    _runSliderLastMoved = now();
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
  printf("------------------------------------------------------------ starting...\n");
  printf("!!! selectRun(%d) running=%d\n", index, _running);
  if (_running) {
    printf("!!! selectRun(%d) running=%d waiting to stop...\n", index, _running);
    setStatus("Waiting for stop...");
    while (_running) {
      d_sleep(0.2);
    }
  }
  printf("!!! selectRun(%d) configuring...\n", index);
  configure_run();
  printf("------------------------------------------------------------ done.\n");
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

  _stopped = true;

  Ami::Calib::set_experiment(basename(std::string(qPrintable(_curdir)).c_str()));

  // Collect all the runs in this dir.
  _runList.clear();
  _runName = "";
  glob_t g;
  QString gpath = _curdir + "/xtc/*-r*-s*.xtc*";
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
  setStatus("Found " + itoa(_runList.size()) + " runs under " + _curdir);
  emit _updateRunCombo();
  if (_runName != "") {
    emit _updateRun();
  }
}

void XtcFileClient::updateDir() { setDir(_curdir); }

void XtcFileClient::runClicked()
{
  setStatus("Run requested...");
  _runButton->setEnabled(false);
  _runName = _runCombo->currentText();
  _task->call(this);
}

void XtcFileClient::pauseClicked() {
  if (!_running) return;

  if (!_paused)
  {
    setStatus("Pause requested...");
    _paused = true;
    emit _setPauseButtonLabel("Resume");
  }
  else
  {
    setStatus("Resume requested...");
    _paused = false;
    emit _setPauseButtonLabel("Pause");
  }
}

void XtcFileClient::stopClicked() {
  if (!_running) return;
  setStatus("Stop requested...");
  _stopped = true;
}

void XtcFileClient::nextEventClicked() {
  if (!_running) return;
  setStatus("nextEvent requested...");
  _iJumpToNextCalib = 0;
  _iJumpToNextEvent = 1;
}

void XtcFileClient::prevEventClicked() {
  if (!_running) return;
  setStatus("prevEvent requested...");
  _iJumpToNextCalib = 0;
  _iJumpToNextEvent = -1;
}

void XtcFileClient::nextCalibClicked() {
  if (!_running) return;
  setStatus("nextCalib requested...");
  _iJumpToNextCalib = 1;
  _iJumpToNextEvent = 1;
}

void XtcFileClient::prevCalibClicked() {
  if (!_running) return;
  setStatus("prevCalib requested...");
  _iJumpToNextCalib = -1;
  _iJumpToNextEvent = 1;
}

void XtcFileClient::jumpToCalibAndEvent() {
  int iCalib = _jumpCalibInput->text().toInt();
  int iEvent = _jumpEventInput->text().toInt();

  if ( iCalib == 0 )
    iCalib = 1;

  if ( iEvent == 0 )
    iEvent = 1;

  if (iCalib > 0 && iEvent > 0)
  {
    _iJumpCalib = iCalib;
    _iJumpEvent = iEvent;
  }

  _jumpMinInput->setText("");
  _jumpSecInput->setText("");
}

void XtcFileClient::jumpToTime() {
  _fJumpMin = _jumpMinInput->text().toDouble();
  _fJumpSec = _jumpSecInput->text().toDouble();

  _jumpCalibInput->setText("");
  _jumpEventInput->setText("");
}

void XtcFileClient::insertTransition(TransitionId::Value transition)
{
  Dgram dg;
  new((void*)&dg.seq) Sequence(Sequence::Event, transition, ClockTime(0,0), TimeStamp(0,0,0,0));
  new((char*)&dg.xtc) Xtc(TypeId(TypeId::Id_Xtc,0),ProcInfo(Level::Recorder,0,0));
  for(unsigned i=0; i<_client.size(); i++)
    _client[i]->processDgram(&dg);
}

void XtcFileClient::routine()
{
  emit _setEnabled(_pauseButton,true);
  emit _setEnabled(_stopButton, true);
  emit _setEnabled(_runButton,  false);
  emit _setEnabled(_runCombo,   false);
  emit _setPauseButtonLabel("Pause");
  emit _setEnabled(_nextEventButton, true);
  emit _setEnabled(_prevEventButton, true);
  emit _setEnabled(_nextCalibButton, true);
  emit _setEnabled(_prevCalibButton, true);

  if (_needs_configure)
    do_configure(_runName);

  if (_runValid) {
    _running = true;
    _stopped = false;
    _paused  = false;
    _needs_configure=true;
    run();
  }

  _paused  = false;
  _stopped = false;
  _running = false;
  emit _setPauseButtonLabel("Pause");
  emit _setEnabled(_nextEventButton, false);
  emit _setEnabled(_prevEventButton, false);
  emit _setEnabled(_nextCalibButton, false);
  emit _setEnabled(_prevCalibButton, false);
  emit _setEnabled(_pauseButton, false);
  emit _setEnabled(_stopButton, false);
  emit _setEnabled(_runButton,  true);
  emit _setEnabled(_runCombo,   true);
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
        _paused  = false;
        _running = false;
        do_configure(_runName);
        if (! _runValid) {
          setStatus("Configuration for run " + _runName + " failed");
        } else if (! testModeConfigOnly && _length > 0) {
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
  setStatus("Fetching files for run " + runName + "...");
  getPathsForRun(files, runName);
  if (files.empty()) {
    setStatus("Found no files paths for run " + runName);
    return;
  }
  setStatus("Fetched " + itoa(files.size()) + " paths for run " + runName);

  _run.live_read(_liveReadMode); // These files are already completely written
  _run.reset(files);

  setStatus("Initializing run " + runName + "...");
  _run.init();

  Ami::Calib::set_run(runName.toInt());

  ClockTime clockStart;
  ClockTime clockEnd;
  if (_run.getStartAndEndTime(clockStart, clockEnd) == 0) {
    _start = clockStart.asDouble();
    _end = clockEnd.asDouble();
    _length = _end - _start;
  } else {
    printf("Could not fetch run start/end times\n");
    _start = 0;
    _end = 0;
    _length = 0;
  }

  setStatus("Fetching first datagram for run " + runName);
  _dgCount = 0;
  insertTransition(TransitionId::Map);

  Dgram* dg = NULL;
  Result result = _run.next(dg);
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
  _executionStart = now();
  if (_length <= 0) {
    _start = clockTime.asDouble();
  }
  _damageMask = 0;
  _damageCount = 0;
  _payloadTotal = 0;

  emit _printDgram(*dg);

  setStatus("Configuring run " + runName);
  for(unsigned i=0; i<_client.size(); i++)
    _client[i]->processDgram(dg);
  for(unsigned i=0; i<_client.size(); i++) {
    _client[i]->wait();
    _client[i]->discover_wait();
  }

  setStatus("Finished configuring run " + runName + ".");
  _runValid = true;
  _needs_configure = false;
}

int XtcFileClient::seekTime(double time) {
  uint32_t seconds     = (uint32_t) time;
  uint32_t nanoseconds = (uint32_t) ((time - seconds) * 1e9);
  int iCalib = -1;
  int iEvent = -1;
  bool bExactMatch = false;
  bool bOvertime = false;
  if (_run.findTime(seconds, nanoseconds, iCalib, iEvent, bExactMatch, bOvertime) != 0) {
    setStatus("Failed to find event for " + dtoa(time - _start, 1) + " seconds after start");
    return 1;
  }
  int eventNum;
  if (_run.jump(iCalib, iEvent, eventNum) != 0) {
    setStatus("Failed to jump to event for " + dtoa(time - _start, 1) + " seconds after start");
    return 2;
  }

  setStatus("Jump to event for " + dtoa(time - _start, 1) + " seconds after start");
  return 0;
}

int XtcFileClient::jumpToNext(int iJumpToNextCalib, int iJumpToNextEvent)
{
  int iNewEventGlobal   = -1;
  int iNewCalib         = -1;
  int iNewEventInCalib  = -1;

  int iError = _run.nextCalibEventId(iJumpToNextCalib, iJumpToNextEvent, true, iNewEventGlobal, iNewCalib, iNewEventInCalib);
  if (iError != 0)
  {
    setStatus("Failed to find the relatve Calib " + itoa(iJumpToNextCalib) + " Event " + itoa(iJumpToNextEvent));
    return 1;
  }

  int iFinalEventNum;
  if (_run.jump(iNewCalib, iNewEventInCalib, iFinalEventNum) != 0)
  {
    setStatus("Failed to jump to Calib " + itoa(iNewCalib) + " Event " + itoa(iNewEventInCalib));
    return 2;
  }

  return 0;
}

void XtcFileClient::run()
{
  _executionStart = now();
  _damageMask = 0;
  _damageCount = 0;
  _payloadTotal = 0;

  double lastPrintTime = 0.0;

  Dgram* last_dg = NULL;
  Dgram* last_printed_dg = NULL;

  while (! _stopped) {
    if (_paused)
    {
      if (! (_stopped || (_runSliderMovedTo >= 0) ||
              (_iJumpCalib > 0 && _iJumpEvent > 0) ||
              (_fJumpMin >= 0 && _fJumpSec >= 0)   ||
              (_iJumpToNextCalib != 0 || _iJumpToNextEvent != 0)
            )
         )
      {
        d_sleep(0.1);
        continue;
      }
    }

    if (_runSliderMovedTo >= 0) {
      int iError = seekTime(_start + _runSliderMovedTo);
      _runSliderMovedTo = -1;
      if (iError != 0 && _paused)
        continue;
    }

    if (_fJumpMin >= 0 && _fJumpSec >= 0)
    {
      int iError = seekTime(_start + _fJumpMin * 60 + _fJumpSec);
      _fJumpMin = -1;
      _fJumpSec = -1;
      if (iError != 0 && _paused)
        continue;
    }

    if (_iJumpCalib > 0 && _iJumpEvent > 0)
    {
      int eventNum;
      int iError = _run.jump(_iJumpCalib, _iJumpEvent, eventNum);
      if (iError  != 0)
        setStatus("Failed to jump to Calib " + itoa(_iJumpCalib) + " Event " + itoa(_iJumpEvent));
      else
        setStatus("Jumped to Calib " + itoa(_iJumpCalib) + " Event " + itoa(_iJumpEvent) + " (Global Event " + itoa(eventNum) + ")");
      _iJumpCalib = -1;
      _iJumpEvent = -1;
      if (iError != 0 && _paused)
        continue;
    }

    if (_iJumpToNextCalib != 0 || _iJumpToNextEvent != 0)
    {
      int iError = jumpToNext(_iJumpToNextCalib, _iJumpToNextEvent);
      _iJumpToNextCalib = 0;
      _iJumpToNextEvent = 0;
      if (iError != 0 && _paused)
        continue;
    }

    Dgram* dg = NULL;
    Result result = _run.next(dg);
    if (result != OK) {
      break;
    }
    last_dg = dg;
    _dgCount++;
    _payloadTotal += dg->xtc.sizeofPayload();

    TransitionId::Value transition = dg->seq.service();
    if (transition==TransitionId::L1Accept) {
      _client[_iclient++]->processDgram(dg);
      if (_iclient == _client.size()) _iclient=0;
    }
    else {
      for(unsigned i=0; i<_client.size(); i++)
        _client[i]->processDgram(dg);
    }

    uint32_t damage = dg->xtc.damage.value();
    if (damage) {
        _damageCount++;
        _damageMask |= damage;
    }

    assert(transition != TransitionId::Configure);
    if (transition == TransitionId::L1Accept) {
      double now = ::now();
      if (lastPrintTime < now - secBetweenPrintDgram) {
        emit _printDgram(*dg);
        lastPrintTime = now;
        last_printed_dg = dg;
      }

      int hz = _hzSliderValue;
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
