#include "ami/qt/DetectorSave.hh"
#include "ami/qt/QtTopWidget.hh"
#include "ami/qt/Path.hh"

#include <QtGui/QFileDialog>

#include <time.h>

using namespace Ami::Qt;

DetectorSave::DetectorSave(QWidget* parent,
			   QtTopWidget** clients,
			   const char** names,
			   int          n) :
  DetectorGroup("Save Plots",parent,clients,names,n) 
{
}

DetectorSave::~DetectorSave()
{
}

void DetectorSave::_init()
{
  char time_buffer[32];
  time_t seq_tm = time(NULL);
  strftime(time_buffer,32,"%Y%m%d_%H%M%S",localtime(&seq_tm));

  QString def = QString("%1/%2").arg(Path::base()).arg(time_buffer);

  _prefix = QFileDialog::getSaveFileName(0,"Save Files with Prefix",
					 def,"*.dat");
}

void DetectorSave::_apply(QtTopWidget& client, const QString& name)
{
  if (!_prefix.isNull())
    client.save_plots(QString("%1_%2").arg(_prefix).arg(name));
}
