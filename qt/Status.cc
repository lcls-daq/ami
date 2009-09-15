#include "Status.hh"

#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>

using namespace Ami::Qt;

Status::Status() :
  QWidget(0),
  _state (Disconnected)
{
  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(_label = new QLabel("Disconnected",this));
  setLayout(layout);
}

Status::~Status()
{
}

void Status::set_state(State s)
{
  _state = s;
  switch(s) {
  case Disconnected: _label->setText("Disconnected"); break;
  case Connected   : _label->setText("Connected"); break;
  case Discovered  : _label->setText("Discovered"); break;
  case Configured  : _label->setText("Configured"); break;
  case Described   : _label->setText("Described"); break;
  case Requested   : _label->setText("Requested"); break;
  case Received    : _label->setText("Received"); break;
  case Processed   : _label->setText("Processed"); break;
  default: break;
  }
}
