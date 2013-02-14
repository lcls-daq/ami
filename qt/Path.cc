#include "Path.hh"

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

#include <time.h>
#include <errno.h>

using namespace Ami::Qt;

static QString _base("ami");

void Path::setBase(const QString& p) { _base=p; }

const QString& Path::base() { return _base; }

FILE* Path::helpFile()
{
  //  QString fname = QString("%1/help.dat").arg(_base);
  QString fname("/reg/g/pcds/dist/pds/misc/ami_help.dat");

  if (fname.isNull())
    return 0;

  return fopen(qPrintable(fname),"r");
}

FILE* Path::saveDataFile(QWidget* parent)
{
  FILE* f = 0;

  char time_buffer[32];
  time_t seq_tm = time(NULL);
  strftime(time_buffer,32,"%Y%m%d_%H%M%S",localtime(&seq_tm));

  QString def = QString("%1/%2.dat").arg(_base).arg(time_buffer);

  QString fname =
    QFileDialog::getSaveFileName(parent,
                                 "Save File As (.dat)",
				 def,"*.dat");

  if (fname.isNull())
    return f;

  if (!fname.endsWith(".dat"))
    fname.append(".dat");

  f = fopen(qPrintable(fname),"w");
  if (!f)
    QMessageBox::warning(parent, 
                         "Save data",
			 QString("Error opening %1 for writing").arg(fname));

  return f;
}

FILE* Path::saveReferenceFile(QWidget* parent,const QString& base) 
{
  QString ref_dir(_base);
  ref_dir += base;
  QString file = QFileDialog::getSaveFileName(parent,
                                              "Reference File:",
					      ref_dir, "*.ref");
  if (file.isNull())
    return 0;

  if (!file.endsWith(".ref"))
    file.append(".ref");

  FILE* f = fopen(qPrintable(file),"w");
  if (!f)
    QMessageBox::critical(parent, "Save Reference", QString("Error opening %s for writing.").arg(file));

  return f; 
}

void Path::saveAmiFile(QWidget* parent, const char* p, int len)
{
  char time_buffer[32];
  time_t seq_tm = time(NULL);
  strftime(time_buffer,32,"%Y%m%d_%H%M%S",localtime(&seq_tm));

  QString def = QString("%1/%2.ami").arg(Path::base()).arg(time_buffer);
  QString fname =     
    QFileDialog::getSaveFileName(parent,"Save Setup to File (.ami)",
                                 def,"*.ami");
  FILE* o = fopen(qPrintable(fname),"w");
  if (o) {
    fwrite(p,len,1,o);
    fclose(o);
    printf("Saved %d bytes to %s\n",len,qPrintable(fname));
  }
  else {
    QString msg = QString("Error opening %1 : %2").arg(fname).arg(strerror(errno));
    QMessageBox::critical(parent,"Save Error",msg);
  }
}

QString Path::loadReferenceFile(QWidget* parent,
                                const QString& base)
{
  QString ref_dir(_base);
  ref_dir += QString(base);
  QString file = QFileDialog::getOpenFileName(parent,
                                              "Reference File:",
					      ref_dir, "*.ref;;*.dat");
  if (file.isNull()) {
    printf("load_reference file is null\n");
  }

  return file;
}
