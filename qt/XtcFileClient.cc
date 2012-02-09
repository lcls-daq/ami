#include <dirent.h>
#include <fcntl.h>
#include <glob.h>
#include <iostream>
#include <string>

#include <QtGui/QApplication>
#include <QtGui/QFileDialog>
#include <QtGui/QVBoxLayout>

#include "ami/qt/XtcFileClient.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/ana/XtcRun.hh"

using namespace Ami::Qt;
using namespace Pds;
using namespace std;

void XtcFileClient::printTransition(Pds::TransitionId::Value transition) {
  if (_lastTransition != transition) {
    cout << "$$$ " << TransitionId::name(transition) << endl;
    _transitionLabel->setText(TransitionId::name(transition));
    _lastTransition = transition;
  }
}

static double getTimeAsDouble() {
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts); // vs CLOCK_PROCESS_CPUTIME_ID
  return ts.tv_sec + ts.tv_nsec / 1.e9;
}

void XtcFileClient::printDgram(const Dgram* dg) {
  char buf[256];

  double now = getTimeAsDouble();
  double deltaSec = now - _runStart;
  double effectiveHz = _dgCount / deltaSec;

  sprintf(buf, "Time: %8d/%8d", dg->seq.stamp().fiducials(), dg->seq.stamp().ticks());
  _timeLabel->setText(buf);

  sprintf(buf, "Datagram count: %d", _dgCount);
  _countLabel->setText(buf);

  sprintf(buf, "Payload size: %8d", dg->xtc.sizeofPayload());
  _payloadSizeLabel->setText(buf);

  sprintf(buf, "Damage: count = %d, mask = %x", _damageCount, _damageMask);
  _damageLabel->setText(buf);

  sprintf(buf, "Rate: %.0f Hz", effectiveHz);
  _hzLabel->setText(buf);

  const ClockTime& clock = dg->seq.clock();

  time_t time = (time_t) clock.seconds();
  char date[256];
  strcpy(date, ctime(&time));

#if 0
static void dump(Dgram* dg)
{
  char buff[128];
  time_t t = dg->seq.clock().seconds();
  strftime(buff,128,"%H:%M:%S",localtime(&t));
  printf("%s %08x/%08x %s extent %x damage %x\n",
   buff,
   dg->seq.stamp().fiducials(),dg->seq.stamp().vector(),
   Pds::TransitionId::name(dg->seq.service()),
   dg->xtc.extent, dg->xtc.damage.value());
}
#endif


  date[strlen(date) - 1] = '\0';
  sprintf(buf, "Clock: %s (start + %.3fs)", date, now - _runStart);
  _clockLabel->setText(buf);
}

void XtcFileClient::getPathsFromRun(QStringList& list, QString run) {
  if (_curdir.isEmpty() || run.isEmpty()) {
    cout << "addPathsFromRun: no paths to add" << endl;
    cout << "dir=" << qPrintable(_curdir) << endl;
    cout << "run=" << qPrintable(run) << endl;
    return;
  }
  cout << "_curdir=[" << qPrintable(_curdir) << "] in addPathsFromRun" << endl;

  QString pattern = _curdir + "/xtc/*-r" + run + "-s*.xtc";
  glob_t g;
  cout << "addPathsFromRun: " << qPrintable(pattern) << endl;
  glob(qPrintable(pattern), 0, 0, &g);

  for (unsigned i = 0; i < g.gl_pathc; i++) {
    char *path = g.gl_pathv[i];
    cout << path << endl;
    list << path;
  }
  globfree(&g);

  list.sort();
}

XtcFileClient::XtcFileClient(QGroupBox* groupBox, Ami::XtcClient& client, const char* curdir) :
  QWidget (0,::Qt::Window),
  _client(client),
  _curdir(curdir),
  _task(new Task(TaskObject("amiqt"))),
  _dir_select(new QPushButton("Select Directory...")),
  _dirLabel(new QLabel(curdir)),
  _run_list(new QComboBox()),

  _runButton(new QPushButton("Run")),
  _stopButton(new QPushButton("Stop")),
  _exitButton(new QPushButton("Exit")),

  _transitionLabel(new QLabel),
  _clockLabel(new QLabel),
  _timeLabel(new QLabel),
  _countLabel(new QLabel),
  _payloadSizeLabel(new QLabel),
  _damageLabel(new QLabel),
  _hzLabel(new QLabel),

  _hzSpinBox(new QSpinBox),
  _loopCheckBox(new QCheckBox("Loop over run")),

  _running(false),
  _stopped(false),
  _lastTransition(TransitionId::Unmap),

  _damageMask(0),
  _damageCount(0)
{
  set_dir(QString(_curdir));
  _hzSpinBox->setRange(1, 240);
  _hzSpinBox->setValue(60);
  _hzSpinBox->setSuffix(" Hz");

  QVBoxLayout* l = new QVBoxLayout;
  l->addWidget(_dir_select);
  l->addWidget(_dirLabel);
  l->addWidget(_run_list);

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
  _transitionLabel->setFont(qfont);
  _clockLabel->setFont(qfont);
  _timeLabel->setFont(qfont);
  _countLabel->setFont(qfont);
  _payloadSizeLabel->setFont(qfont);
  _damageLabel->setFont(qfont);
  _hzLabel->setFont(qfont);

  l->addWidget(_transitionLabel);
  l->addWidget(_clockLabel);
  l->addWidget(_timeLabel);
  l->addWidget(_countLabel);
  l->addWidget(_payloadSizeLabel);
  l->addWidget(_damageLabel);
  l->addWidget(_hzLabel);

  l->addWidget(_hzSpinBox);
  l->addWidget(_loopCheckBox);

  groupBox->setLayout(l);

  connect(_dir_select, SIGNAL(clicked()), this, SLOT(select_dir()));
  connect(_runButton, SIGNAL(clicked()), this, SLOT(run_clicked()));
  connect(_stopButton, SIGNAL(clicked()), this, SLOT(stop_clicked()));
  connect(_exitButton, SIGNAL(clicked()), qApp, SLOT(closeAllWindows()));
  connect(_run_list, SIGNAL(currentIndexChanged(int)), this, SLOT(select_run(int)));
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

void XtcFileClient::select_run(int index)
{
  _stopped = true;
  if (_running) {
    cout << "Waiting for stop..." << endl;
    while (_running) {
      d_sleep(0.2);
    }
  }
  configure();
}

void XtcFileClient::select_dir()
{
  set_dir(QFileDialog::getExistingDirectory(0, "Select Directory", _curdir, 0));
}

void XtcFileClient::set_dir(QString dir)
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
  _dirLabel->setText(dir);

  // Clear the list widget and the current run
  cout << "_curdir is " << qPrintable(_curdir) << endl;
  cout << "_run_list->currentText() was " << qPrintable(_run_list->currentText()) << endl;
  _run_list->clear();

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
  _run_list->addItems(runs);

  printf("Found %d runs\n", runs.size());
}

void XtcFileClient::run_clicked() 
{
  _runButton->setEnabled(false);
  _task->call(this); 
}

void XtcFileClient::stop_clicked() {
  cout << "stop requested..." << endl;
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
  _stopButton->setEnabled(true);
  _runButton->setEnabled(false);
  _running = true;
  _stopped = false;

  do {
    run(false);
  } while (! _stopped && _loopCheckBox->isChecked());

  _stopped = false;
  _running = false;
  _stopButton->setEnabled(false);
  _runButton->setEnabled(true);
}

void XtcFileClient::configure()
{
  run(true);
}

bool XtcFileClient::run(bool configure_only)
{
  QStringList files;
  cout << "_curdir is " << qPrintable(_curdir) << endl;
  getPathsFromRun(files, _run_list->currentText());
  if (files.empty()) {
    cout << "openXtcRun(): No xtc files found in " << qPrintable(_curdir) << endl;
    return false;
  }

  XtcRun& run = _run;
  string file = qPrintable(files.first());
  run.reset(file);
  files.pop_front();
  while (! files.empty()) {
    file = qPrintable(files.first());
    run.add_file(file);
    files.pop_front();
  }
  run.init();
  
  _runStart = getTimeAsDouble();
  _dgCount = 0;
  _damageMask = 0;
  _damageCount = 0;

  insertTransition(TransitionId::Map);

  double lastPrintTime = 0.0;

  while (configure_only || ! _stopped) {
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
    _client.processDgram(dg);

    uint32_t damage = dg->xtc.damage.value();
    if (damage) {
        _damageCount++;
        _damageMask |= damage;
    }

    if (transition == TransitionId::L1Accept) {
      double now = getTimeAsDouble();
      if (lastPrintTime < now - 0.1) {
        printDgram(dg);
        lastPrintTime = now;
      }

      int hz = _hzSpinBox->value();
      if (hz != 0) {
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
    } else if (transition == TransitionId::Configure && configure_only) {
      break;
    }
  }

  insertTransition(TransitionId::Unconfigure);
  insertTransition(TransitionId::Unmap);
  return true;
}
