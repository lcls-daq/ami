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
    layout->addLayout(layout1); }
  setLayout(layout);

  connect(_loBox , SIGNAL(textEdited(const QString&)), this, SLOT(changeLoEdge(const QString&)));
  connect(_hiBox , SIGNAL(textEdited(const QString&)), this, SLOT(changeHiEdge(const QString&)));
  connect(_autoB , SIGNAL(clicked(bool)), this, SLOT(auto_scale(bool)));

  _autoB->setCheckable(true);   // initialize with auto scale
  _autoB->setChecked  (true);
}   

AxisControl::~AxisControl()
{
}

void AxisControl::save(char*& p) const
{
  QtPersistent::insert(p,_loBox->text().toDouble());
  QtPersistent::insert(p,_hiBox->text().toDouble());
  QtPersistent::insert(p,_autoB->isChecked());
}

void AxisControl::load(const char*& p)
{
  _loBox->setText(QtPersistent::extract_s(p));
  _hiBox->setText(QtPersistent::extract_s(p));
  _autoB->setChecked(QtPersistent::extract_b(p));
}

void   AxisControl::update(const AxisInfo& info) 
{
  _info = &info;
  updateInfo();
}

bool   AxisControl::isAuto() const { return _autoB->isChecked(); }

double AxisControl::loEdge() const { return _loBox->text().toDouble(); }

double AxisControl::hiEdge() const { return _hiBox->text().toDouble(); }

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


