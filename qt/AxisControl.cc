#include "AxisControl.hh"
#include "AxisInfo.hh"

#include "ami/qt/QtPersistent.hh"

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
  _logB  = new QPushButton("Log Scale"); 
  _loBox = new QLineEdit("0");
  _hiBox = new QLineEdit("1000");
  _loBox->setMaximumWidth(60);
  _hiBox->setMaximumWidth(60);
  new QDoubleValidator(_loBox);
  new QDoubleValidator(_hiBox);

  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(_loBox);
    layout1->addStretch();
    layout1->addWidget(_autoB);
    layout1->addStretch();
    layout1->addWidget(_hiBox);
    layout1->addStretch();
    layout1->addWidget(_logB);
    layout->addLayout(layout1); }
  setLayout(layout);

  connect(_loBox , SIGNAL(textEdited(const QString&)), this, SLOT(changeLoEdge(const QString&)));
  connect(_hiBox , SIGNAL(textEdited(const QString&)), this, SLOT(changeHiEdge(const QString&)));
  connect(_autoB , SIGNAL(clicked(bool)), this, SLOT(auto_scale(bool)));
  connect(_logB  , SIGNAL(clicked(bool)), this, SLOT(log_scale(bool)));

  _autoB->setCheckable(true);   // initialize with auto scale
  _autoB->setChecked  (true);

  _logB->setCheckable(true);
  _logB->setChecked  (false);
}   

AxisControl::~AxisControl()
{
}

void AxisControl::save(char*& p) const
{
  QtPersistent::insert(p,_loBox->text());
  QtPersistent::insert(p,_hiBox->text());
  QtPersistent::insert(p,_autoB->isChecked());
  QtPersistent::insert(p,_logB ->isChecked());
}

void AxisControl::load(const char*& p)
{
  QString s,t;
  bool b;
  _loBox->setText(s=QtPersistent::extract_s(p));
  _hiBox->setText(t=QtPersistent::extract_s(p));
  _autoB->setChecked(b=QtPersistent::extract_b(p));
  auto_scale(b);
  _logB ->setChecked(b=QtPersistent::extract_b(p));
  log_scale(b);
}

void   AxisControl::update(const AxisInfo& info) 
{
  _info = &info;
  updateInfo();
}

bool   AxisControl::isAuto() const { return _autoB->isChecked(); }
bool   AxisControl::isLog () const { return _logB ->isChecked(); }

double AxisControl::loEdge() const {
  double v = _loBox->text().toDouble(); 
  if (isLog() && v<=0) v = 0.5;
  return v;
}

double AxisControl::hiEdge() const {
  double v = _hiBox->text().toDouble(); 
  if (isLog() && v<=0) v = 5.0;
  return v;
}

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
  emit windowChanged();
}

void AxisControl::log_scale(bool l)
{
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


