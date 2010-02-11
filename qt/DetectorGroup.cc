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
			     const std::list<QtTopWidget*>& clients) :
  QtPWidget(parent),
  _clients (clients),
  _snapshot(clients),
  _buttons (new QButtonGroup)
{
  _buttons->setExclusive(false);

  setWindowTitle(label);
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QVBoxLayout* l = new QVBoxLayout;

  int i=0;
  for(std::list<QtTopWidget*>::const_iterator it = _snapshot.begin();
      it != _snapshot.end(); it++,i++) {
    QCheckBox* button = new QCheckBox(it->title(),this);
    button->setChecked(false);
    button->setEnabled(false);
    l->addWidget(button);
    _buttons->addButton(button,i++);
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
  int i=0;
  for(std::list<QtTopWidget*>::const_iterator it = _snapshot.begin();
      it != _snapshot.end(); it++,i++) {
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
    int i=0;
    for(std::list<QtTopWidget*>::const_iterator it = _snapshot.begin();
	it != _snapshot.end(); it++,i++) {
      if (it->name() == name) {
	QAbstractButton* box = _buttons->button(i);
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

void DetectorGroup::update_list()
{
  QButtonGroup* g = new QButtonGroup;
  QVBoxLayout*  l = new QVBoxLayout;

  int i=0;
  for(std::list<QtTopWidget*>::const_iterator it = _clients.begin();
      it != _clients.end(); it++,i++) {
    QCheckBox* newbox = new QCheckBox(it->title(),this);
    g->addButton(newbox,i);
    l->addWidget(newbox);
    int j=0;
    for(std::list<QtTopWidget*>::const_iterator sit = _snapshot.begin();
	sit != _snapshot.end(); sit++,j++) {
      if (*it == *sit) {
	QAbstractButton* box = _buttons->button(j);
	newbox->setChecked(box->isChecked());
      }
    }
  }
  delete layout();
  setLayout(l);
}

void DetectorGroup::apply()
{
  _init();
  int i=0;
  for(std::list<QtTopWidget*>::const_iterator it = _snapshot.begin();
      it != _snapshot.end(); it++,i++) {
    QAbstractButton* box = _buttons->button(i);
    if (box->isEnabled() && box->isChecked()) {
      QtTopWidget* p = *it;
      _apply(*p);
    }
  }
}

void DetectorGroup::close()
{
  hide();
}