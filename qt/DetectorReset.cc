#include "ami/qt/DetectorReset.hh"
#include "ami/qt/QtTopWidget.hh"

using namespace Ami::Qt;

DetectorReset::DetectorReset(QWidget* parent,
			     QtTopWidget** clients,
			     const char** names,
			     int          n) :
  DetectorGroup("Reset Plots",parent,clients,names,n) 
{
}

DetectorReset::~DetectorReset()
{
}

void DetectorReset::_apply(QtTopWidget& client, const QString&)
{
  client.reset_plots();
}
