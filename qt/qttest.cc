#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include <QtGui/QApplication>

#include "ami/qt/Cursors.hh"

using namespace Ami::Qt;

int main(int argc, char **argv) 
{
  QApplication app(argc, argv);

//   Filter* f = new Filter("Ch1");
//   f->show();

//   Transform* t = new Transform("Test : X Transform","x");
//   t->show();

//   ChannelDefinition* ch = new ChannelDefinition("ChT");
//   ch->show();

//  MyAxisInfo info(0,100,0.,1.);

//   AxisControl* a = new AxisControl(0, "X", info);
//   a->show();

//  TClient* w = new TClient;
//  w->show();

  Cursors* c = new Cursors(2);
  c->show();

  app.exec();

  return 0;
}
