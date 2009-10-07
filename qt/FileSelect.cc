#include "FileSelect.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QDateEdit>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>

#include <glob.h>
#include <libgen.h>
#include <string>

using namespace Ami::Qt;

FileSelect::FileSelect(QWidget* parent,
		       const QStringList& paths) :
  QWidget(parent),
  _paths (paths),
  _list  (new QListWidget),
  _date  (new QDateEdit)
{
  _date->setDisplayFormat("yyyy.MM.dd");
  QVBoxLayout* l = new QVBoxLayout;
  l->addWidget(_date);
  l->addWidget(_list);
  setLayout(l);

  connect(_date,  SIGNAL(dateChanged(const QDate&)),
	  this ,  SLOT  (change_date(const QDate&)));
  connect(_list,  SIGNAL(currentRowChanged(int)),
	  this,   SLOT  (file_selected(int)));

  _date->setDate(QDate::currentDate());
}

FileSelect::~FileSelect()
{
}

QStringList FileSelect::paths() const
{
  QStringList v;
  if (!_file.isEmpty()) {
    glob_t g;
    for(QStringList::const_iterator it=_paths.constBegin(); 
	it != _paths.constEnd(); it++) {
      QString file = *it + "/" + _file;
      std::string gpath(qPrintable(file));
      glob(gpath.c_str(),0,0,&g);
      if (g.gl_pathc)
	v << file;
    }
    globfree(&g);
  }
  return v;
}

void FileSelect::change_path_list(const QStringList& paths)
{
  _paths = paths;
  change_date(_date->date());
}

void FileSelect::change_date(const QDate& date)
{
  disconnect(_list, SIGNAL(currentRowChanged(int)),
	     this,  SLOT  (file_selected(int)));

  _list->clear();

  char dbuff[64];
  sprintf(dbuff,"/%4d%02d%02d-*.xtc",date.year(),date.month(),date.day());

  QStringList files;

  for(QStringList::const_iterator it=_paths.constBegin(); it != _paths.constEnd(); it++) {
    glob_t g;
    std::string gpath(qPrintable(*it));
    gpath += dbuff;
    printf("Trying %s\n",gpath.c_str());
    glob(gpath.c_str(),0,0,&g);
    for(unsigned i=0; i<g.gl_pathc; i++) {
      QString s(basename(g.gl_pathv[i]));
      if (!files.contains(s))
	files << s;
    }
    globfree(&g);
  }

  files.sort();
  _list->addItems(files);
  _file = QString("");

  printf("Found %d runs\n",files.size());

  connect(_list, SIGNAL(currentRowChanged(int)),
	  this,  SLOT  (file_selected(int)));
}

void FileSelect::file_selected(int row)
{
  if (row>=0)
    _file = _list->currentItem()->text();
  else
    _file = QString("");
}
