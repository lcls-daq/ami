#include "ami/qt/Control.hh"

#include "ami/qt/Client.hh"
#include "ami/service/Task.hh"

#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>

using namespace Ami::Qt;

Control::Control(Requestor& c) :
  QWidget(0),
  _client(c),
  _task  (new Task(TaskObject("amitmr")))
{
  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(_pRun    = new QPushButton("Run")); _pRun->setCheckable(true);
  layout->addWidget(_pSingle = new QPushButton("Single"));
  setLayout(layout);

  connect(_pRun   , SIGNAL(clicked(bool)), this, SLOT(run (bool)));
  connect(_pSingle, SIGNAL(clicked()), this, SLOT(single()));
}

Control::~Control()
{
  _task->destroy();
}

void Control::expired() { _client.request_payload(); }

Ami::Task* Control::task() { return _task; }

unsigned Control::duration() const { return 400; }

unsigned Control::repetitive() const { return _repeat; }

void Control::run (bool l) {
  _client.one_shot(false);
  _repeat=1; 
  if (l)  Timer::start(); 
  else    cancel();
}

void Control::single() {
  _client.one_shot(true);
  _repeat = 0; 
  Timer::start();
}

void Control::save(char*& p) const
{
  QtPersistent::insert(p,_pRun   ->isChecked());
}

void Control::load(const char*& p)
{
  bool b = QtPersistent::extract_b(p);
  printf("Extract RUN state %c\n",b?'t':'f');
  _pRun   ->setChecked(b);
  run(b);
}
