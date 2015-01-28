#include "ImageColorControl.hh"

#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QStackedWidget>

#include <math.h>
#include <vector>
#include <stdio.h>

using namespace Ami::Qt;

static ColorMaps _colorMaps;

static QVector<QRgb>* get_palette_from_colormap(unsigned int colormap[])
{
  QVector<QRgb>* color_table = new QVector<QRgb>(256);
  for (int i = 0; i < 256; i++) {
    unsigned int rgb = colormap[i];
    unsigned int r = (rgb & 0xff0000) >> 16;
    unsigned int g = (rgb & 0xff00) >> 8;
    unsigned int b = (rgb & 0xff);
    color_table->insert(i, qRgb(r, g, b));
  }
  return color_table;
}

static QVector<QRgb>* jet_palette()
{
  return get_palette_from_colormap(_colorMaps.get("jet"));
}

static QVector<QRgb>* monochrome_palette()
{
  return get_palette_from_colormap(_colorMaps.get("gray"));
}

static QVector<QRgb>* thermal_palette()
{
  QVector<QRgb>* color_table = new QVector<QRgb>(256);
  for (int i = 0; i < 43; i++)  // black - red
    color_table->insert(  0+i, qRgb(i*6,0,0));
  for (int i = 0; i < 86; i++)  // red - green
    color_table->insert( 43+i, qRgb(255-i*3,i*3,0));
  for (int i = 0; i < 86; i++)  // green - blue
    color_table->insert(129+i, qRgb(0,255-i*3,i*3));
  for (int i = 0; i < 40; i++)  // blue - violet
    color_table->insert(215+i, qRgb(i*3,0,255-i*3));
  color_table->insert(255, qRgb(255,255,255));
  return color_table;
}

class PaletteSet {
public:
  PaletteSet() {
    _v.push_back(monochrome_palette());
    _v.push_back(thermal_palette());
  }
public:
  unsigned num_palettes() const { return _v.size(); }
  const QVector<QRgb>& palette(int i) { return *_v[i]; }
public:
  std::string names() const {
    std::string n("mono");
    n += ",jet";
    n += ",thermal";
    return n;
  }
  bool parse_string(const char* opt) {
    _v.clear();
    
    QString qopt(opt);
    QStringList qlist = qopt.split(",",::QString::SkipEmptyParts);
    for(int i=0; i<qlist.size(); i++) {
      if      (qlist[i].compare("mono",::Qt::CaseInsensitive)==0)
        _v.push_back(monochrome_palette());
      else if (qlist[i].compare("jet",::Qt::CaseInsensitive)==0)
        _v.push_back(jet_palette());
      else if (qlist[i].compare("thermal",::Qt::CaseInsensitive)==0)
        _v.push_back(thermal_palette());
      else if (qlist[i].compare("0")==0) {
        _v.push_back(monochrome_palette());
        _v.push_back(jet_palette());
      }
      else if (qlist[i].compare("1")==0) {
        _v.push_back(monochrome_palette());
        _v.push_back(thermal_palette());
      }
      else
        return false;
    }

    return (_v.size()>0);
  }
private:
  std::vector< QVector<QRgb>* > _v;
};

static PaletteSet _palettes;

string ImageColorControl::palette_set()
{
  return _palettes.names();
}

bool ImageColorControl::parse_palette_set(const char* opt)
{
  return _palettes.parse_string(opt);
}

const QVector<QRgb>& ImageColorControl::current_color_table()
{
  return _palettes.palette(_palettes.num_palettes()-1);
}

ImageColorControl::ImageColorControl(QWidget* parent,
				     const QString&  title) :
  QGroupBox(title,parent),
  _scale_min     (new QLineEdit),
  _scale_mid     (new QLabel),
  _scale_max     (new QLineEdit),
  _range         (Fixed)
{
  for(unsigned i=0; i<NRanges; i++) {
    _scale   [i] = 1;
    _pedestal[i] = 0;
  }

  _scale_min->setMaximumWidth(60);
  _scale_min->setValidator(new QDoubleValidator(_scale_min));
  _scale_min->setText(QString::number(_pedestal[_range]));

  _scale_max->setMaximumWidth(60);
  _scale_max->setValidator(new QDoubleValidator(_scale_max));
  //  _scale_max->setText(QString::number(_pedestal[_range]));

  _scale_min->setAlignment(::Qt::AlignLeft);
  _scale_mid->setAlignment(::Qt::AlignHCenter);
  _scale_max->setAlignment(::Qt::AlignRight);

  setAlignment(::Qt::AlignHCenter);

  QPushButton* resetB = new QPushButton("Reset");
  QPushButton* zoomB  = new QPushButton("Zoom");
  QPushButton* panB   = new QPushButton("Pan");

  QImage palette(256,16,QImage::Format_Indexed8);
  { unsigned char* dst = palette.bits();
    for(unsigned k=0; k<256*16; k++) *dst++ = k&0xff; }

  std::vector<QRadioButton*> paletteButtons(_palettes.num_palettes());
  std::vector<QLabel*      > palettePixmaps(_palettes.num_palettes());
  _paletteGroup = new QButtonGroup;
  for(unsigned i=0; i<_palettes.num_palettes(); i++) {
    palette.setColorTable(_palettes.palette(i));
    paletteButtons[i] = new QRadioButton;
    (palettePixmaps[i] = new QLabel)->setPixmap(QPixmap::fromImage(palette));
    _paletteGroup->addButton(paletteButtons[i], i);
  }

  _color_table = new QVector<QRgb>;
  *_color_table = _palettes.palette(_palettes.num_palettes()-1);
  paletteButtons[_palettes.num_palettes()-1]->setChecked(true);

  QStringList ranges;
  ranges << "Fixed";
  ranges << "Full";
  ranges << "Dynamic";
  QComboBox* rangebox  = new QComboBox;
  rangebox->addItems(ranges);
  rangebox->setCurrentIndex(0);

  QStackedWidget* stack = new QStackedWidget;
  { _logscale_fixed = new QCheckBox("Log Scale");
    _logscale_fixed->setChecked(false);
    stack->addWidget(_logscale_fixed); }
  { _logscale_full = new QCheckBox("Log Scale");
    _logscale_full->setChecked(false);
    stack->addWidget(_logscale_full); }
  { QWidget* w = new QWidget;
    QHBoxLayout* h = new QHBoxLayout;
    _nsigma = new QLineEdit("3");
    _nsigma->setMaximumWidth(30);
    *new QDoubleValidator(_nsigma);
    h->addWidget(_nsigma);
    h->addWidget(new QLabel("sigma"));
    w->setLayout(h);
    stack->addWidget(w); }

  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(zoomB);
    layout1->addWidget(resetB);
    layout1->addWidget(panB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    { QGridLayout* layout2 = new QGridLayout;
      unsigned i;
      for(i=0; i<_palettes.num_palettes(); i++) {
        layout2->addWidget(paletteButtons[i],i,0);
        layout2->addWidget(palettePixmaps[i],i,1,1,3);
      }
      layout2->addWidget(_scale_min,i,1,::Qt::AlignLeft);
      layout2->addWidget(_scale_mid,i,2,::Qt::AlignCenter); 
      layout2->addWidget(_scale_max,i,3,::Qt::AlignRight); 
      //  Set the columns to equal (dynamic) size
      layout2->setColumnMinimumWidth(1,60);
      layout2->setColumnMinimumWidth(2,60);
      layout2->setColumnMinimumWidth(3,60);
      layout2->setColumnStretch(1,1);
      layout2->setColumnStretch(2,1);
      layout2->setColumnStretch(3,1);
      layout1->addLayout(layout2); }
    layout1->addStretch();
    layout->addLayout(layout1); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(rangebox);
    layout1->addWidget(stack);
    layout->addLayout(layout1); }
  setLayout(layout);

  connect(resetB, SIGNAL(clicked(bool)), this, SLOT(reset(bool)));
  connect(zoomB , SIGNAL(clicked()), this, SLOT(zoom()));
  connect(panB  , SIGNAL(clicked()), this, SLOT(pan ()));
  connect(_paletteGroup, SIGNAL(buttonClicked(int)), this, SLOT(set_palette(int)));
  connect(_scale_min, SIGNAL(editingFinished()), this, SLOT(scale_changed()));
  connect(_scale_max, SIGNAL(editingFinished()), this, SLOT(scale_changed()));
  connect(_logscale_fixed, SIGNAL(clicked()), this, SIGNAL(windowChanged()));
  connect(_logscale_full , SIGNAL(clicked()), this, SIGNAL(windowChanged()));
  connect(this  , SIGNAL(windowChanged()), this, SLOT(show_scale()));
  connect(this  , SIGNAL(scaleChanged()), this, SLOT(show_scale()));
  connect(rangebox, SIGNAL(currentIndexChanged(int)), stack, SLOT(setCurrentIndex(int)));
  connect(rangebox, SIGNAL(currentIndexChanged(int)), this, SLOT(range_changed(int)));
  show_scale();
  _range_box = rangebox;
}   

ImageColorControl::~ImageColorControl()
{
  delete _paletteGroup;
}

void ImageColorControl::save(char*& p) const
{
  XML_insert(p, "double", "_scale"   , QtPersistent::insert(p,&_scale   [0], NRanges));
  XML_insert(p, "double", "_pedestal", QtPersistent::insert(p,&_pedestal[0], NRanges));
  XML_insert(p, "QButtonGroup", "_paletteGroup", QtPersistent::insert(p,_paletteGroup->checkedId()) );
  XML_insert(p, "QCheckBox", "_logscale_fixed", QtPersistent::insert(p,_logscale_fixed->isChecked()) );
  XML_insert(p, "QCheckBox", "_logscale_full" , QtPersistent::insert(p,_logscale_full->isChecked()) );
  XML_insert(p, "QComboBox", "_range_box" , QtPersistent::insert(p,_range_box->currentIndex()) );
}

void ImageColorControl::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.name == "_scale")
      QtPersistent::extract_d(p,_scale);
    else if (tag.name == "_pedestal")
      QtPersistent::extract_d(p,_pedestal);
    else if (tag.name == "_paletteGroup") {
      int palette = QtPersistent::extract_i(p);
      _paletteGroup->button(palette)->setChecked(true);
      set_palette(palette);
    }
    else if (tag.name == "_logscale_fixed")
      _logscale_fixed->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_logscale_full")
      _logscale_full ->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_range_box")
      _range_box->setCurrentIndex(QtPersistent::extract_i(p));
  XML_iterate_close(ImageColorControl,tag);

  show_scale();
  emit windowChanged();
}

bool   ImageColorControl::linear() const 
{
  switch(_range) {
  case Fixed   : return !_logscale_fixed->isChecked();
  case Full    : return !_logscale_full ->isChecked();
  default:       break;
  }
  return true;
}

float ImageColorControl::pedestal() const { return _pedestal[_range]; }

float ImageColorControl::scale() const { return _scale[_range]; }

const QVector<QRgb>& ImageColorControl::color_table() const { return *_color_table; }

void   ImageColorControl::set_palette(int p)
{
  if (p >= (int) _palettes.num_palettes()) p = _palettes.num_palettes()-1;
  *_color_table = _palettes.palette(p);

  emit windowChanged();
}

void   ImageColorControl::show_scale()
{
  unsigned v = static_cast<unsigned>(0xff*scale());
  double   p = _pedestal[_range];
  if (!linear()) {
    _scale_min->setText(QString::number(p+1));
    _scale_mid->setText(QString::number(p+int(sqrt(v)+0.5)));
    _scale_max->setText(QString::number(p+v));
  }
  else {
    _scale_min->setText(QString::number(p));
    _scale_mid->setText(QString::number(p+(v>>1)));
    _scale_max->setText(QString::number(p+v));
  }
}

void   ImageColorControl::reset(bool s)
{
  _pedestal[_range] = 0;
  _scale   [_range] = 1;
  emit windowChanged();
}

void   ImageColorControl::zoom ()
{
  _scale[_range] /= sqrt(2);
  emit windowChanged();
}

void   ImageColorControl::pan ()
{
  _scale[_range] *= sqrt(2);
  emit windowChanged();
}

void   ImageColorControl::scale_changed()
{
  _pedestal[_range] = _scale_min->text().toDouble();
  _scale   [_range] = (_scale_max->text().toDouble()-_pedestal[_range])/255.;
  emit windowChanged();
}

void   ImageColorControl::range_changed(int m)
{
  _range = (Range)m;
  emit windowChanged();
}

ImageColorControl::Range   ImageColorControl::range() const { return _range; }

void ImageColorControl::full_range_setup(double zmin, double zmax)
{
  _pedestal[Full] = zmin;
  _scale   [Full] = (zmax-zmin)/255.;
  emit scaleChanged();
}

void ImageColorControl::dynamic_range_setup(double n, double sum, double sqsum)
{
  double mean = sum/n;
  double rang = sqrt(sqsum/n - mean*mean);
  double nsgm = _nsigma->text().toDouble();
  rang *= nsgm;
  _pedestal[Dynamic] = mean-rang;
  _scale   [Dynamic] = 2*rang/255.;
  emit scaleChanged();
}
