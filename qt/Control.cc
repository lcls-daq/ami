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
  QPushButton* pRun;
  QPushButton* pSingle;
  
  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(pRun    = new QPushButton("Run")); pRun->setCheckable(true);
  layout->addWidget(pSingle = new QPushButton("Single"));
  setLayout(layout);

  connect(pRun   , SIGNAL(clicked(bool)), this, SLOT(run (bool)));
  connect(pSingle, SIGNAL(clicked()), this, SLOT(single()));
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

