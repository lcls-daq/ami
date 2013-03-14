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

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget( _run );
  layout->addWidget( _grid       );
  layout->addWidget( _minor_grid );
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

