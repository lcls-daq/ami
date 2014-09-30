#include "ami/qt/Defaults.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>

#include <stdio.h>

using namespace Ami::Qt;

Defaults::Defaults() 
{
  setWindowTitle("Defaults");

  setAttribute(::Qt::WA_DeleteOnClose, false);

  _run        = new QCheckBox("Select 'Run'"   );  _run       ->setChecked(true);
  _grid       = new QCheckBox("Show Grid"      );  _grid      ->setChecked(false);
  _minor_grid = new QCheckBox("Show Minor Grid");  _minor_grid->setChecked(false);
  
  (_image_rate  = new QLineEdit("1"))->setMaximumWidth(40);
  new QDoubleValidator(_image_rate);
  (_others_rate = new QLineEdit("2.5"))->setMaximumWidth(40);
  new QDoubleValidator(_others_rate);

  (_plot_width  = new QLineEdit("200"))->setMaximumWidth(40);
  new QIntValidator(_plot_width);
  (_plot_height = new QLineEdit("200"))->setMaximumWidth(40);
  new QIntValidator(_plot_height);

  _movie_format_box = new QComboBox;
  _movie_format_box->addItem("jpg");
  _movie_format_box->addItem("png");
  _movie_format_box->addItem("tiff");
  _movie_format_box->setCurrentIndex(0);

  _save_precision = new QComboBox;
  for(unsigned i=0; i<16; i++)
    _save_precision->addItem(QString::number(i));
  _save_precision->setCurrentIndex(15);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget( _run );
  layout->addWidget( _grid       );
  layout->addWidget( _minor_grid );

  { QGroupBox* g = new QGroupBox("Update Rates");
    QGridLayout* l = new QGridLayout;
    l->addWidget(new QLabel("Images"),0,0,::Qt::AlignRight);
    l->addWidget(_image_rate,0,1);
    l->addWidget(new QLabel("Hz"),0,2,::Qt::AlignLeft);
    l->addWidget(new QLabel("Others"),1,0,::Qt::AlignRight);
    l->addWidget(_others_rate,1,1);
    l->addWidget(new QLabel("Hz"),1,2,::Qt::AlignLeft);
    l->setColumnStretch(2,1);
    g->setLayout(l);
    layout->addWidget(g); }

  { QGroupBox* g = new QGroupBox("Plot Size");
    QGridLayout* l = new QGridLayout;
    l->addWidget(new QLabel("Width"),0,0,::Qt::AlignRight);
    l->addWidget(_plot_width,0,1);
    l->addWidget(new QLabel("pixels"),0,2,::Qt::AlignLeft);
    l->addWidget(new QLabel("Height"),1,0,::Qt::AlignRight);
    l->addWidget(_plot_height,1,1);
    l->addWidget(new QLabel("pixels"),1,2,::Qt::AlignLeft);
    l->setColumnStretch(2,1);
    g->setLayout(l);
    layout->addWidget(g); }

  { QHBoxLayout* hl = new QHBoxLayout;
    hl->addWidget(new QLabel("Save data precision"));
    hl->addWidget(_save_precision);
    layout->addLayout(hl); }
  { QHBoxLayout* hl = new QHBoxLayout;
    hl->addWidget(new QLabel("Movie format"));
    hl->addWidget(_movie_format_box);
    layout->addLayout(hl); }

  setLayout(layout);
}

Defaults::~Defaults()
{
}

bool Defaults::select_run     () const { return _run ->isChecked      (); }
bool Defaults::show_grid      () const { return _grid->isChecked      (); }
bool Defaults::show_minor_grid() const { return _minor_grid->isChecked(); }
double Defaults::image_update_rate() const { return _image_rate->text().toDouble(); }
double Defaults::other_update_rate() const { return _others_rate->text().toDouble(); }
int  Defaults::plot_width () const { return _plot_width ->text().toInt(); }
int  Defaults::plot_height() const { return _plot_height->text().toInt(); }
int  Defaults::save_precision () const { return _save_precision->currentIndex(); }
QString Defaults::movie_format() const { return _movie_format_box->currentText(); }

void Defaults::save(char*& p) const
{
  XML_insert( p, "bool", "select_run", QtPersistent::insert(p,_run->isChecked()));
  XML_insert( p, "bool", "show_grid" , QtPersistent::insert(p,_grid->isChecked()));
  XML_insert( p, "bool", "show_minor_grid", QtPersistent::insert(p,_minor_grid->isChecked()));
  XML_insert( p, "double", "image_rate", QtPersistent::insert(p,_image_rate->text().toDouble()));
  XML_insert( p, "double", "others_rate", QtPersistent::insert(p,_others_rate->text().toDouble()));
  XML_insert( p, "int", "plot_width" , QtPersistent::insert(p,_plot_width ->text().toInt()));
  XML_insert( p, "int", "plot_height", QtPersistent::insert(p,_plot_height->text().toInt()));
  XML_insert( p, "int", "save_precision", QtPersistent::insert(p,_save_precision->currentIndex()));
  XML_insert( p, "int", "movie_format", QtPersistent::insert(p,_movie_format_box->currentIndex()));
}

void Defaults::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if      (tag.name == "select_run")
      _run->setChecked( QtPersistent::extract_b(p) );
    else if (tag.name == "show_grid")
      _grid->setChecked( QtPersistent::extract_b(p) );
    else if (tag.name == "show_minor_grid")
      _minor_grid->setChecked( QtPersistent::extract_b(p) );
    else if (tag.name == "image_rate")
      _image_rate->setText( QString::number(QtPersistent::extract_d(p)) );
    else if (tag.name == "others_rate")
      _others_rate->setText( QString::number(QtPersistent::extract_d(p)) );
    else if (tag.name == "plot_width")
      _plot_width->setText( QString::number(QtPersistent::extract_i(p)) );
    else if (tag.name == "plot_height")
      _plot_height->setText( QString::number(QtPersistent::extract_i(p)) );
    else if (tag.name == "save_precision")
      _save_precision->setCurrentIndex( QtPersistent::extract_i(p) );
    else if (tag.name == "movie_format")
      _movie_format_box->setCurrentIndex( QtPersistent::extract_i(p) );
  XML_iterate_close(ImageClient,tag);
}

static Defaults* _instance = 0;

Defaults* Defaults::instance()
{
  if (!_instance)
    _instance = new Defaults;

  return _instance;
}

