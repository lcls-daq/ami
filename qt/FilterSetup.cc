#include "ami/qt/FilterSetup.hh"

#include "ami/client/ClientManager.hh"
#include "ami/data/Discovery.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>

using namespace Ami::Qt;

FilterSetup::FilterSetup(ClientManager& manager) :
  _manager(manager)
{
  setWindowTitle("Event Filters");

  setAttribute(::Qt::WA_DeleteOnClose, false);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget( _list = new QListWidget(this) );
  { QHBoxLayout* l = new QHBoxLayout;
    QPushButton* applyB = new QPushButton("Apply");
    QPushButton* closeB = new QPushButton("Close");
    l->addWidget(applyB);
    l->addWidget(closeB);
    layout->addLayout(l); 
    connect(applyB, SIGNAL(clicked()), this, SLOT(apply()));
    connect(closeB, SIGNAL(clicked()), this, SLOT(hide ()));
  }

  setLayout(layout);
}

FilterSetup::~FilterSetup()
{
}

void FilterSetup::update(const Ami::DiscoveryRx& rx)
{
  int n = _list->count();
  for(unsigned i=0; i<rx.features(); i++) {
    const char* name = rx.feature_name(i);
    if (strncmp(name,"UF:",3)==0) {
      QListWidgetItem* item = new QListWidgetItem(tr(name+3),_list);
      for(int j=0; j<n; j++) {
        QListWidgetItem* o = _list->item(j);
        if (o->text()==item->text())
          item->setCheckState(o->checkState());
      }
    }
  }
  for(int i=0; i<n; i++)
    delete _list->takeItem(0);
}

unsigned FilterSetup::selected() const
{
  unsigned result = 0;
  for(int i=0; i<_list->count(); i++)
    if (_list->item(i)->checkState()==::Qt::Checked)
      result |= (1<<i);
  return result;
}

void FilterSetup::save(char*& p) const
{
}

void FilterSetup::load(const char*& p)
{
}

void FilterSetup::apply()
{
  _manager.configure();
}
