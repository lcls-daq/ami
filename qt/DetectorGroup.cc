#include "ami/qt/DetectorGroup.hh"
#include "ami/qt/QtTopWidget.hh"

#include <QtGui/QButtonGroup>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>

using namespace Ami::Qt;

DetectorGroup::DetectorGroup(const QString& label,
			     QWidget*     parent,
			     QtTopWidget** clients,
			     const char** names,
			     int          n) :
  QtPWidget(parent),
  _clients (clients),
  _n       (n),
  _buttons (new QButtonGroup)
{
  _buttons->setExclusive(false);

  setWindowTitle(label);
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QVBoxLayout* l = new QVBoxLayout;

  for(int i=0; i<n; i++) {
    QCheckBox* button = new QCheckBox(names[i],this);
    button->setChecked(false);
    button->setEnabled(false);
    l->addWidget(button);
    _buttons->addButton(button,i);
  }

  { QHBoxLayout* layout = new QHBoxLayout;
    QPushButton* applyB = new QPushButton("Apply");
    QPushButton* closeB = new QPushButton("Close");
    layout->addWidget(applyB);
    layout->addWidget(closeB);
    l->addLayout(layout); 
    connect(applyB, SIGNAL(clicked()), this, SLOT(apply()));
    connect(closeB, SIGNAL(clicked()), this, SLOT(close())); }

  setLayout(l);
}

DetectorGroup::~DetectorGroup()
{
}

void DetectorGroup::save(char*& p) const
{
  QtPWidget::save(p);
  for(int i=0; i<_n; i++) {
    QAbstractButton* box = _buttons->button(i);
    QtPersistent::insert(p, box->text());
    QtPersistent::insert(p, box->isChecked());
  }
  QtPersistent::insert(p, QString("EndGroup"));
}

void DetectorGroup::load(const char*& p)
{
  QtPWidget::load(p);
  QString name = QtPersistent::extract_s(p);
  while(name!=QString("EndGroup")) {
    for(int i=0; i<_n; i++) {
      QAbstractButton* box = _buttons->button(i);
      if (box->text() == name) {
	box->setChecked(QtPersistent::extract_b(p));
	break;
      }
    }
    name = QtPersistent::extract_s(p);
  }
}

void DetectorGroup::enable(int i)
{
  QAbstractButton* box = _buttons->button(i);
  box->setEnabled(true);
  box->setChecked(true);
}

void DetectorGroup::disable(int i)
{
  QAbstractButton* box = _buttons->button(i);
  box->setEnabled(false);
  box->setChecked(false);
}

void DetectorGroup::apply()
{
  _init();
  for(int i=0; i<_n; i++) {
    QAbstractButton* box = _buttons->button(i);
    if (box->isEnabled() && box->isChecked())
      _apply(*_clients[i],box->text());
  }
}

void DetectorGroup::close()
{
  hide();
}
