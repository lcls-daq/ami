#include "AxisControl.hh"
#include "AxisInfo.hh"

#include <QtGui/QScrollBar>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QDoubleValidator>

using namespace Ami::Qt;


AxisControl::AxisControl(QWidget* parent,
			 const QString&  title) :
  QGroupBox(title,parent),
  _info  (0)
{
  setAlignment(::Qt::AlignHCenter);

  _autoB = new QPushButton("Auto"); 
  _loBox = new QLineEdit("0");
  _hiBox = new QLineEdit("1000");
  _loBox->setMaximumWidth(60);
  _hiBox->setMaximumWidth(60);
  new QDoubleValidator(_loBox);
  new QDoubleValidator(_hiBox);

#ifdef USE_SCROLL
  _zoomB = new QPushButton("Zoom");
  _panB  = new QPushButton("Pan");

  _scroll = new QScrollBar(::Qt::Horizontal);
  _scroll->setRange(info.lo(),info.lo());
  _scroll->setPageStep(info.hi()-info.lo());

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(_scroll);
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(_loBox);
    layout1->addStretch();
    layout1->addWidget(new QLabel("-"));
    layout1->addStretch();
    layout1->addWidget(_hiBox);
    layout->addLayout(layout1); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(_zoomB);
    layout1->addWidget(_autoB);
    layout1->addWidget(_panB);
    layout->addLayout(layout1); }
  setLayout(layout);

  connect(_scroll, SIGNAL(valueChanged(int)), this, SLOT(changeLoEdge(int)));
  connect(_zoomB , SIGNAL(clicked()), this, SLOT(zoom()));
  connect(_panB  , SIGNAL(clicked()), this, SLOT(pan ()));
#else

  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(_loBox);
    layout1->addStretch();
    layout1->addWidget(_autoB);
    layout1->addStretch();
    layout1->addWidget(_hiBox);
    layout->addLayout(layout1); }
  setLayout(layout);

#endif
  connect(_loBox , SIGNAL(textEdited(const QString&)), this, SLOT(changeLoEdge(const QString&)));
  connect(_hiBox , SIGNAL(textEdited(const QString&)), this, SLOT(changeHiEdge(const QString&)));
  connect(_autoB , SIGNAL(clicked(bool)), this, SLOT(auto_scale(bool)));

  _autoB->setCheckable(true);   // initialize with auto scale
  _autoB->setChecked  (true);
}   

AxisControl::~AxisControl()
{
}

void   AxisControl::update(const AxisInfo& info) 
{
  _info = &info;
  updateInfo();
}

bool   AxisControl::isAuto() const { return _autoB->isChecked(); }

double AxisControl::loEdge() const { return _loBox->text().toDouble(); }

double AxisControl::hiEdge() const { return _hiBox->text().toDouble(); }

#ifdef USE_SCROLL
void AxisControl::updateInfo()
{
  int ilo = _info->tick(_loBox->text().toDouble());
  int ihi = _info->tick(_hiBox->text().toDouble());
  if (ilo < _info->lo()) ilo = _info->lo();
  if (ihi > _info->hi()) ihi = _info->hi();

  _scroll->setRange(_info->lo(),_info->hi()-ihi+ilo);
  _scroll->setValue   (ilo);
  _scroll->setPageStep(ihi-ilo);

  _loBox->setText(QString::number(_info->position(ilo)));
  _hiBox->setText(QString::number(_info->position(ihi)));
}

// user moved scroll
void AxisControl::changeLoEdge(int ilo) {
  int ihi = ilo + _scroll->pageStep();
  
  _loBox->setText(QString::number(_info->position(ilo)));
  _hiBox->setText(QString::number(_info->position(ihi)));

  emit windowChanged(ilo, ihi);
}

// loBox was edited
void AxisControl::changeLoEdge(const QString& t)
{
  int ilo = _info->tick(t.toDouble());
  int ihi = _scroll->value()+_scroll->pageStep();

  if (ilo > _info->hi()) {
    ilo = ihi;
    ihi = _info->hi();
  }
  else if (ilo < _info->lo()) {
    ilo = _info->lo();
  }

  changeWindow(ilo,ihi);
}

// hiBox was edited
void AxisControl::changeHiEdge(const QString& t)
{
  int ilo = _scroll->value();
  int ihi = _info->tick(t.toDouble());

  if (ihi < _info->lo()) {
    ihi = ilo;
    ilo = _info->lo();
  }
  else if (ihi > _info->hi()) {
    ihi = _info->hi();
  }

  changeWindow(ilo,ihi);
}

// zoom was pressed
void AxisControl::zoom()
{
  int istep = _scroll->pageStep()/2;
  int ilo   = _scroll->value() + istep/2;
  int ihi   = ilo + istep;
  
  changeWindow(ilo,ihi);
}

// pan was pressed
void AxisControl::pan ()
{
  int istep = _scroll->pageStep();
  int ilo   = _scroll->value() - istep/2;
  if (ilo < _info->lo()) 
    ilo = _info->lo();
  int ihi   = ilo + 2*istep;
  if (ihi > _info->hi()) {
    ihi = _info->hi();
    ilo = ihi - 2*istep;
    if (ilo < _info->lo())
      ilo = _info->lo();
  }
  
  changeWindow(ilo,ihi);
}

void AxisControl::changeWindow(int ilo, int ihi)
{
  _scroll->setRange(_info->lo(),_info->hi()-ihi+ilo);
  _scroll->setValue   (ilo);
  _scroll->setPageStep(ihi-ilo);

  _loBox->setText(QString::number(_info->position(ilo)));
  _hiBox->setText(QString::number(_info->position(ihi)));

  emit windowChanged(ilo, ihi);
}

void AxisControl::auto_scale(bool l)
{
  _loBox ->setEnabled(!l);
  _hiBox ->setEnabled(!l);
  _zoomB ->setEnabled(!l);
  _panB  ->setEnabled(!l);
  if (l)
    changeWindow(_info->lo(), _info->hi());
}
#else

// loBox was edited
void AxisControl::changeLoEdge(const QString& t)
{
  emit windowChanged();
}

// hiBox was edited
void AxisControl::changeHiEdge(const QString& t)
{
  emit windowChanged();
}

void AxisControl::auto_scale(bool l)
{
//   _loBox ->setEnabled(!l);
//   _hiBox ->setEnabled(!l);
  emit windowChanged();
}

void AxisControl::updateInfo()
{
  if (isAuto()) {
//     _loBox->setText(QString::number(_info->position(_info->lo())));
//     _hiBox->setText(QString::number(_info->position(_info->hi())));
  }
  else {
    int ilo = _info->tick(_loBox->text().toDouble());
    int ihi = _info->tick(_hiBox->text().toDouble());
    if (ilo < _info->lo()) ilo = _info->lo();
    if (ihi > _info->hi()) ihi = _info->hi();

    _loBox->setText(QString::number(_info->position(ilo)));
    _hiBox->setText(QString::number(_info->position(ihi)));
  }
}
#endif


