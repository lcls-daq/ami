#include "Path.hh"

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

#include <time.h>

using namespace Ami::Qt;

FILE* Path::saveDataFile()
{
  FILE* f = 0;

  char time_buffer[32];
  time_t seq_tm = time(NULL);
  strftime(time_buffer,32,"%Y%m%d_%H%M%S",localtime(&seq_tm));

  QString def("ami/data/");
  def += time_buffer;
  def += ".dat";
  QString fname =
    QFileDialog::getSaveFileName(0,"Save File As (.dat)",
				 def,".dat");
  if (!fname.isNull()) {
    f = fopen(qPrintable(fname),"w");
    if (!f)
      QMessageBox::warning(0, "Save data",
			   QString("Error opening %1 for writing").arg(fname));
  }

  return f;
}

FILE* Path::saveReferenceFile(const QString& base) 
{
  QString ref_dir("ami/ref/");
  ref_dir += base;
  QString file = QFileDialog::getSaveFileName(0,"Reference File:",
					      ref_dir, "*.ref");
  if (file.isNull())
    return 0;

  if (!file.contains("."))
    file.append(".ref");

  FILE* f = fopen(qPrintable(file),"w");
  if (!f)
    QMessageBox::critical(0, "Save Reference", QString("Error opening %s for writing.").arg(file));

  return f; 
}

QString Path::loadReferenceFile(const QString& base)
{
  QString ref_dir("ami/ref/");
  ref_dir += QString(base);
  QString file = QFileDialog::getOpenFileName(0,"Reference File:",
					      ref_dir, "*.ref");
  if (file.isNull()) {
    printf("load_reference file is null\n");
  }

  return file;
}
