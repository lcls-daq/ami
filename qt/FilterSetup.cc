#include "ami/qt/FilterSetup.hh"

#include "ami/qt/Filter.hh"
#include "ami/client/ClientManager.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/RawFilter.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>

#define DBUG

using namespace Ami::Qt;

static const QString _expr("Expression");
static Ami::RawFilter _no_filter;

FilterSetup::FilterSetup(ClientManager& manager) :
  _manager(manager)
{
  setWindowTitle("Event Filters");

  setAttribute(::Qt::WA_DeleteOnClose, false);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget( _list = new QListWidget(this) );
  connect(_list, SIGNAL(itemActivated(QListWidgetItem*)), 
	  this , SLOT(activated(QListWidgetItem*)));
  { QHBoxLayout* l = new QHBoxLayout;
    QPushButton* applyB  = new QPushButton("Apply");
    QPushButton* closeB  = new QPushButton("Close");
    l->addWidget(applyB);
    l->addWidget(closeB);
    layout->addLayout(l); 
    connect(applyB , SIGNAL(clicked()), this, SLOT(apply()));
    connect(closeB , SIGNAL(clicked()), this, SLOT(hide ()));
  }

  (new QListWidgetItem(_expr,_list))->setCheckState(::Qt::Unchecked);
  setLayout(layout);

  _filter =new Filter(this, "EventFilter");
  connect(_filter, SIGNAL(changed()), this, SLOT(apply()));
}

FilterSetup::~FilterSetup()
{
  delete _filter;
}

void FilterSetup::update(const Ami::DiscoveryRx& rx)
{
  int n = _list->count();

  (new QListWidgetItem(_expr,_list))->setCheckState(_list->item(0)->checkState());

  const Pds::DetInfo noInfo;
  const Ami::DescEntry* nxt;
  for(const Ami::DescEntry* e = rx.entries(); e < rx.end(); e = nxt) {
    nxt = reinterpret_cast<const Ami::DescEntry*>
      (reinterpret_cast<const char*>(e) + e->size());
    
    if (e->info().level() == Pds::Level::Event) {
      const char* name = e->name();
      ::Qt::CheckState check(::Qt::Unchecked);
      QString qname(name);
      for(int j=0; j<n; j++) {
        QListWidgetItem* o = _list->item(j);
        if (o->text()==qname) {
          check = o->checkState();
          break;
        }
      }
      (new QListWidgetItem(qname,_list))->setCheckState(check);
    }
  }

  for(int i=0; i<n; i++)
    delete _list->takeItem(0);
}

unsigned FilterSetup::selected() const
{
  unsigned result = 0;
  for(int i=1; i<_list->count(); i++)
    if (_list->item(i)->checkState()==::Qt::Checked)
      result |= (1<<i);
  result >>= 1;
  return result;
}

void FilterSetup::save(char*& p) const
{
  XML_insert(p, "Filter", "_filter", _filter->save(p) );
  for(int i=0; i<_list->count(); i++)
    if (_list->item(i)->checkState()==::Qt::Checked)
      XML_insert(p, "QString", "item", QtPersistent::insert(p, _list->item(i)->text()));
}

void FilterSetup::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.name == "_filter")
      _filter->load(p);
    else if (tag.name == "item") {
      QString name = QtPersistent::extract_s(p);
      for(int i=0; i<_list->count(); i++)
	if (_list->item(i)->text() == name)
	  _list->item(i)->setCheckState(::Qt::Checked);
    }
  XML_iterate_close(FilterSetup,tag);
  apply();
 }

void FilterSetup::apply()
{
  _manager.configure();
}

void FilterSetup::activated(QListWidgetItem* item)
{
#ifdef DBUG
  printf("FS:activated %p %p\n",item,_list->item(0));
#endif
  if (item == _list->item(0)) {
    _filter->front();
  }
}

const Ami::AbsFilter& FilterSetup::filter() const
{
  return (_list->item(0)->checkState()==::Qt::Checked) ?
    *_filter->filter() : _no_filter;
}
