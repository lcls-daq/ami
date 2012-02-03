#include <dirent.h>
#include <glob.h>
#include <iostream>
#include <string>

#include <QtGui/QApplication>
#include <QtGui/QFileDialog>
#include <QtGui/QVBoxLayout>

#include "ami/qt/QtMonitorServer.hh"
#include "ami/qt/XtcFileClient.hh"
#include "pdsdata/xtc/ProcInfo.hh"

using namespace Ami::Qt;
using namespace Pds;
using namespace std;

// TODO:
// 1. hz should be configurable

//#define CLOCK CLOCK_PROCESS_CPUTIME_ID
#define CLOCK CLOCK_REALTIME

static void printTransition0(const Dgram* dg) {
#if 0
  printf("%18s transition: time %08x/%08x, payloadSize 0x%08x dmg 0x%x\n",
         TransitionId::name(dg->seq.service()),
         dg->seq.stamp().fiducials(),
         dg->seq.stamp().ticks(),
         dg->xtc.sizeofPayload(),
         dg->xtc.damage.value());
#else
  printf("%18s transition: fiducials %8d ticks %8d payload size %8d damage %d\n",
         TransitionId::name(dg->seq.service()),
         dg->seq.stamp().fiducials(),
         dg->seq.stamp().ticks(),
         dg->xtc.sizeofPayload(),
         dg->xtc.damage.value());
#endif
}

void XtcFileClient::printTransition(const Dgram* dg, const double hz) {
  _transitionLabel->setText(TransitionId::name(dg->seq.service()));

  char buf[64];

#if 0
  sprintf(buf, "Time: %08x/%08x", dg->seq.stamp().fiducials(), dg->seq.stamp().ticks());
  _timeLabel->setText(buf);

  sprintf(buf, "Payload size: 0x%08x", dg->xtc.sizeofPayload());
  _payloadSizeLabel->setText(buf);

  sprintf(buf, "Damage: 0x%x", dg->xtc.damage.value());
  _damageLabel->setText(buf);
#else
  sprintf(buf, "Time: %8d/%8d", dg->seq.stamp().fiducials(), dg->seq.stamp().ticks());
  _timeLabel->setText(buf);

  sprintf(buf, "Payload size: %8d", dg->xtc.sizeofPayload());
  _payloadSizeLabel->setText(buf);

  sprintf(buf, "Damage: %d", dg->xtc.damage.value());
  _damageLabel->setText(buf);
#endif
}


// Internal method called when we have come to the
// end of one run, and need to read more files to
// start the next run. Returns true if we are
// out of files.
bool XtcFileClient::_skipToNextRun() {
  if (_paths.empty()) {
    return false;
  }
  cout << endl << "Adding files for new run..." << endl;
  _run.reset(_paths.front());
  cout << "Adding " << _paths.front() << endl;
  _paths.pop_front();
  while (! _paths.empty() && _run.add_file(_paths.front())) {
    cout << "Adding " << _paths.front() << endl;
    _paths.pop_front();
  }
  cout << endl << "calling run.init()..." << endl;
  _run.init();
  cout << endl << "done with run.init()." << endl;
  _runIsValid = true;
  return true;
}

// Takes a new list of paths, sorts it, and then appends the contents
// to the existing list of paths (by moving the new paths).
void XtcFileClient::_addPaths(list<string> newPaths) {
  newPaths.sort();
  _paths.splice(_paths.end(), newPaths);
}

long long int XtcFileClient::timeDiff(struct timespec* end, struct timespec* start) {
  long long int diff;
  diff =  (end->tv_sec - start->tv_sec) * 1000000000;
  diff += end->tv_nsec;
  diff -= start->tv_nsec;
  return diff;
}

// Fetch the next
Dgram* XtcFileClient::next() {
  for (;;) {
    if (! _runIsValid && ! _skipToNextRun()) {
      return NULL;
    }
    Dgram* dg = NULL;
    int iSlice = -1;
    int64_t i64Offset = -1;
    Ana::Result result = _run.next(dg, &iSlice, &i64Offset);
    if (result != Ana::OK) {
      _runIsValid = false;
      continue; // need to skip to next run
    }
    return dg;
  }
}

void XtcFileClient::addPathsFromRun(QString run) {
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

  list<string> newPaths;
  for (unsigned i = 0; i < g.gl_pathc; i++) {
    string path = string(g.gl_pathv[i]);
    cout << path << endl;
    newPaths.push_back(path);
  }
  _addPaths(newPaths);
  globfree(&g);
}

// Constructor that starts with an empty list of paths.
XtcFileClient::XtcFileClient(QGroupBox* groupBox, const char* partitionTag, unsigned interface, unsigned serverGroup, const char* curdir) :
  QWidget (0,::Qt::Window),
  _curdir(curdir),
  _interface(interface),
  _serverGroup(serverGroup),
  _task(new Task(TaskObject("amiqt"))),
  _dir_select(new QPushButton("Select Directory...")),
  _dirLabel(new QLabel(curdir)),
  _file_select(new QComboBox()),

  _runButton(new QPushButton("Run")),
  _stopButton(new QPushButton("Stop")),
  _exitButton(new QPushButton("Exit")),

  _transitionLabel(new QLabel("")),
  _timeLabel(new QLabel("")),
  _payloadSizeLabel(new QLabel("")),
  _damageLabel(new QLabel("")),

  _hzSlider(new QSlider(::Qt::Horizontal)),
  _loopCheckBox(new QCheckBox("Loop over run")),

  _running(false),
  _server(NULL),
  _runIsValid(false),
  _stopped(false)
{
  set_dir(QString(_curdir));
  _hzSlider->setRange(1, 120);

  QVBoxLayout* l = new QVBoxLayout;
  l->addWidget(_dir_select);
  l->addWidget(_dirLabel);
  l->addWidget(_file_select);

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
  _timeLabel->setFont(qfont);
  _payloadSizeLabel->setFont(qfont);
  _damageLabel->setFont(qfont);

  l->addWidget(_transitionLabel);
  l->addWidget(_timeLabel);
  l->addWidget(_payloadSizeLabel);
  l->addWidget(_damageLabel);

  l->addWidget(_loopCheckBox);

  groupBox->setLayout(l);
#if 0
  //  l->addWidget(groupBox);
#else
  //  setLayout(l);
#endif

  connect(_dir_select, SIGNAL(clicked()), this, SLOT(select_dir()));
  connect(_runButton, SIGNAL(clicked()), this, SLOT(run()));
  connect(_stopButton, SIGNAL(clicked()), this, SLOT(stop()));
  connect(_exitButton, SIGNAL(clicked()), qApp, SLOT(closeAllWindows()));
  connect(_loopCheckBox, SIGNAL(stateChanged(int)), this, SLOT(loopCheckBoxChanged(int)));
  connect(this , SIGNAL(done()), this, SLOT(ready()));

  // These are mandatory
  int numberOfBuffers = 4;
  unsigned sizeOfBuffers = 0x800000;

  // These are optional
  int rate = 60; // Hz
  unsigned nclients = 1;
  unsigned sequenceLength = 1;

  if (rate > 0) {
    _period = 1000000000 / rate; // period in nanoseconds
    cout << "Rate is " << rate << " Hz; period is " << _period / 1e6 << " msec" << endl;
  } else {
    _period = 0;
    cout << "Rate was not specified; will run unthrottled." << endl;
  }

  cout << "partitionTag = " << partitionTag << endl;
  cout << "sizeOfBuffers = " << sizeOfBuffers << endl;
  cout << "numberOfBuffers = " << numberOfBuffers << endl;
  cout << "nclients = " << nclients << endl;
  cout << "sequenceLength = " << sequenceLength << endl;

  struct timespec start, now;
  clock_gettime(CLOCK, &start);
  cout << "calling QtMonitorServer..." << endl;
  _server = new QtMonitorServer(partitionTag,
                                sizeOfBuffers, 
                                numberOfBuffers, 
                                nclients,
                                sequenceLength,
                                this);
  cout << "called QtMonitorServer..." << endl;
  clock_gettime(CLOCK, &now);
  printf("Opening shared memory took %.3f msec.\n", timeDiff(&now, &start) / 1e6);
}

XtcFileClient::~XtcFileClient()
{
  _task->destroy();
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
  cout << "_file_select->currentText() was " << qPrintable(_file_select->currentText()) << endl;
  _file_select->clear();

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
  _file_select->addItems(runs);

  printf("Found %d runs\n", runs.size());
}

void XtcFileClient::loopCheckBoxChanged(int state) {
  cout << "loopCheckBox: state = " << state << endl;
}

void XtcFileClient::run() 
{
  _runButton->setEnabled(false);
  _task->call(this); 
}

void XtcFileClient::ready() 
{
  _runButton->setEnabled(true);
}

void XtcFileClient::stop() {
  cout << "stop requested..." << endl;
  _stopped = true;
}

/*
void XtcFileClient::configure()
{
  const char* fname = qPrintable(_file_select->paths().at(0));

  int fd = ::open(fname,O_LARGEFILE,O_RDONLY);
  if (fd == -1) {
    printf("Error opening file %s : %s\n",fname,strerror(errno));
    return;
  }
  else {
    printf("Opened file %s\n",fname);
  }

  char* buffer = new char[0x800000];
  Pds::Dgram* dg = (Pds::Dgram*)buffer;

  insert_transition(dg, TransitionId::Map);
  _client.processDgram(dg);

  //  read configure transition
  ::read(fd,dg, sizeof(Dgram));
  if (::read(fd,dg->xtc.payload(), dg->xtc.sizeofPayload()) != dg->xtc.sizeofPayload()) 
    printf("Unexpected eof in %s\n",fname);
  else
    _client.processDgram(dg);

  insert_transition(dg, TransitionId::Unconfigure);
  _client.processDgram(dg);

  insert_transition(dg, TransitionId::Unmap);
  _client.processDgram(dg);

  ::close(fd);
  delete[] buffer;
}
*/

void XtcFileClient::routine()
{
  _stopButton->setEnabled(true);
  addPathsFromRun(_file_select->currentText());
  _running = true;


  _stopped = false;
  _server->insert(TransitionId::Map);

  Dgram* dg;
  timespec loopStart;
  clock_gettime(CLOCK, &loopStart);
  int dgCount = 0;

  unsigned long long last_fiducial = 0;
  unsigned long long last_tick = 0;

  for (;;) {
    if (_stopped) {
      cout << "stopped" << endl;
      break;
    }
    dg = next();
    if (dg == NULL) {
      cout << "_loopCheckBox->isChecked() = " << _loopCheckBox->isChecked() << endl;
      if (_loopCheckBox->isChecked()) {
        addPathsFromRun(_file_select->currentText());
        dg = next();
        cout << "trying to loop again; dg = " << dg << endl;
      }
      if (dg == NULL) {
        cout << "no more datagrams" << endl;
        break;
      }
    }



      /*
    Xtc* xtc = &dg->xtc;
    Level::Type level = xtc->src.level();
    const DetInfo& info = *(DetInfo*)(&xtc->src);
    if (level==Level::Source) {
      printf("%s level contains: %s: ",Level::name(level), TypeId::name(xtc->contains.id()));
      printf("%s %d %s %d",
             DetInfo::name(info.detector()),info.detId(),
             DetInfo::name(info.device()),info.devId());
      if (xtc->damage.value()) {
        printf(", damage 0x%x", xtc->damage.value());
      }
      printf("\n");
    } else {
      ProcInfo& info = *(ProcInfo*)(&xtc->src);
      printf("%s level contains: %s: ",Level::name(level), TypeId::name(xtc->contains.id()));
      printf("IpAddress 0x%x ProcessId 0x%x",info.ipAddr(),info.processId());
      if (xtc->damage.value()) {
        printf(", damage 0x%x", xtc->damage.value());
      }
      printf("\n");
    }
      */






    /*
    unsigned long long this_fiducial = dg->seq.stamp().fiducials();
    unsigned long long this_tick = dg->seq.stamp().ticks();

    if (this_fiducial < last_fiducial ||
        this_tick < last_tick) {

      cout << "last fiducial / tick: " << last_fiducial << " / " << last_tick << endl;
      cout << "this fiducial / tick: " << this_fiducial << " / " << this_tick << endl;
    }

    last_fiducial = this_fiducial;
    last_tick = this_tick;
    */

    timespec dgStart;
    clock_gettime(CLOCK, &dgStart);
    dgCount++;
    _server->events(dg);


    //      _server->routine();
    if (dg->seq.service() != TransitionId::L1Accept) {
      //printTransition0(dg);
      clock_gettime(CLOCK, &loopStart);
      dgCount = 0;
    } else {
      timespec now;
      clock_gettime(CLOCK, &now);
      double hz = dgCount / (timeDiff(&now, &loopStart) / 1.e9);
      //printTransition0(dg);
      printTransition(dg, hz);
    }

    if (_period != 0) {
      timespec now;
      clock_gettime(CLOCK, &now);
      long long int busyTime = timeDiff(&now, &dgStart);
      if (_period > busyTime) {
        timespec sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_nsec = _period - busyTime;
        if (nanosleep(&sleepTime, &now) < 0) {
          perror("nanosleep");
        }
      }
    }
  }

  _server->insert(TransitionId::Unconfigure);
  _server->insert(TransitionId::Unmap);


  _running = false;
  _stopButton->setEnabled(false);
  emit done();
}
