#include "ami/qt/PWidgetManager.hh"

#include "ami/qt/QtPWidget.hh"

#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>

#include <vector>
#include <list>

using namespace Ami::Qt;

static std::vector<QtPWidget*> _pitems;
static std::vector<QString>    _items;
static std::list<PWidgetManager*> _managers;


PWidgetManager::PWidgetManager(QWidget* parent) :
  QWidget(parent)
{
  QComboBox* box = new QComboBox;
  for(unsigned i=0; i<_items.size(); i++)
    box->addItem(_items[i]);

  QHBoxLayout* l = new QHBoxLayout;
  //  l->addStretch();
  l->addWidget(new QLabel("Find Plot"));
  l->addWidget(box,1);
  setLayout(l);

  connect(box, SIGNAL(activated(int)), this, SLOT(selected(int)));
  _box = box;

  _managers.push_back(this);
}

PWidgetManager::~PWidgetManager()
{
  _managers.remove(this);
}

void PWidgetManager::add(QtPWidget* p, QString& iname)
{
  QString name(iname);
  unsigned j=0;
  for(unsigned i=0; i<_items.size(); i++) {
    if (name == _items[i])
      name = QString("%1_%2").arg(iname).arg(++j);
  }

  unsigned i=0;
  while(i<_items.size() && _items[i] < name)
    i++;

  _pitems.insert(_pitems.begin()+i,p);
  _items .insert(_items .begin()+i,name);

  for(std::list<PWidgetManager*>::iterator it=_managers.begin();
      it != _managers.end();
      it++)
    (*it)->sync();

  iname = name;
}

void PWidgetManager::remove(QtPWidget* p)
{
  unsigned i=0;
  while(i<_items.size() && p!=_pitems[i])
    i++;

  if (i<_items.size()) {

    _pitems.erase(_pitems.begin()+i);
    _items .erase(_items .begin()+i);

    for(std::list<PWidgetManager*>::iterator it=_managers.begin();
        it != _managers.end();
        it++)
      (*it)->sync();
  }
}

void PWidgetManager::sync()
{
  _box->clear();
  for(unsigned i=0; i<_items.size(); i++)
    _box->addItem(_items[i]);
}

void PWidgetManager::selected(int i)
{
  _pitems[i]->front();
}

