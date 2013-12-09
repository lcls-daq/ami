#include "ami/qt/Defaults.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>

using namespace Ami::Qt;

Defaults::Defaults() 
{
  setWindowTitle("Defaults");

  setAttribute(::Qt::WA_DeleteOnClose, false);

  _run        = new QCheckBox("Select 'Run'"   );  _run       ->setChecked(true);
  _grid       = new QCheckBox("Show Grid"      );  _grid      ->setChecked(false);
  _minor_grid = new QCheckBox("Show Minor Grid");  _minor_grid->setChecked(false);
  
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
  layout->addWidget( _save_precision );
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
int  Defaults::save_precision () const { return _save_precision->currentIndex(); }
QString Defaults::movie_format() const { return _movie_format_box->currentText(); }

void Defaults::save(char*& p) const
{
}

void Defaults::load(const char*& p)
{
}

static Defaults* _instance = 0;

Defaults* Defaults::instance()
{
  if (!_instance)
    _instance = new Defaults;

  return _instance;
}

