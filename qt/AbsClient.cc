#include "ami/qt/AbsClient.hh"
#include "ami/qt/Path.hh"
#include <sys/stat.h>

using namespace Ami::Qt;

AbsClient::AbsClient(QWidget*            parent,
		     const Pds::DetInfo& src, 
		     unsigned            channel) :
  QtTopWidget(parent,src,channel) {}

AbsClient::~AbsClient() {}

void AbsClient::beginRun(unsigned) {}

void AbsClient::endRun  (unsigned run) 
{
  if (Path::archive()) {
    QString dir = QString("%1/r%2/").arg(*Path::archive()).arg(run);
    mkdir(qPrintable(dir), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP);
    snapshot(dir);
  }
}
