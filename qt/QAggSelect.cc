#include "ami/qt/QAggSelect.hh"
#include "ami/qt/SMPRegistry.hh"
#include "ami/qt/SMPWarning.hh"
#include "ami/data/QtPersistent.hh"

#include <QtGui/QButtonGroup>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QGridLayout>
#include <QtGui/QRadioButton>
#include <QtGui/QIntValidator>

#include <limits.h>

using namespace Ami::Qt;

enum { Events=0, Rate=1 };

static inline int avgRound(int n, int d)
{
  return n<0 ? -1 : (n+d-1)/d;
}

QAggSelect::QAggSelect() :
  _group           (new QButtonGroup),
  _interval        (new QLineEdit),
  _intervalq       (new QLabel),
  _smp_warning     (new SMPWarning),
  _enabled         (false)
{
  QGridLayout* layout1 = new QGridLayout;
  QRadioButton* events = new QRadioButton("over");
  layout1->addWidget(events,0,0);
  layout1->addWidget(_interval ,0,1);
  layout1->addWidget(_intervalq,0,2);
  layout1->addWidget(new QLabel("Events"),0,3);

  QRadioButton* rate = new QRadioButton("at update rate");
  layout1->addWidget(rate,1,0,1,4);

  layout1->addWidget(_smp_warning,0,4,2,1);

  _group->addButton(events  , Events);
  _group->addButton(rate    , Rate);

  setLayout(layout1);

  new QIntValidator(0,INT_MAX,_interval);

  connect(_interval, SIGNAL(editingFinished()), this, SLOT(update_interval()));
  connect(&SMPRegistry::instance(), SIGNAL(changed()), this, SLOT(update_interval()));

  _group->button(Events)->setChecked(true);
  _intervalq->setEnabled(_enabled);
}

QAggSelect::~QAggSelect()
{
  delete _group;
}

bool QAggSelect::at_rate() const { return _group->checkedId()==Rate; }

unsigned QAggSelect::over_events () const { 
  return avgRound(_interval->text().toUInt(),
                  SMPRegistry::instance().nservers());
}

int QAggSelect::value() const
{
  return _group->checkedId()==Rate ? -1 : over_events();
}

void QAggSelect::enable(bool v)
{
  _enabled = v;
  _interval->setEnabled(v);
  update_interval();
}

void QAggSelect::update_interval()
{
  unsigned nproc = SMPRegistry::instance().nservers();
  int n = _interval->text().toInt();
  int m = nproc*avgRound(n,nproc);
  if (n>1 && m!=n)
    _intervalq->setText(QString("(%1)").arg(QString::number(m)));
  else
    _intervalq->clear();

  bool ldist = _enabled && nproc>1;
  _smp_warning->setEnabled(ldist);
}

void QAggSelect::save(char*& p) const
{
  XML_insert( p, "QButtonGroup", "_group", QtPersistent::insert(p,_group->checkedId()) );
  XML_insert( p, "QLineEdit", "_interval", QtPersistent::insert(p,_interval->text().toInt()) );
}

void QAggSelect::load(const char*& p) 
{
  int id = 0;
  XML_iterate_open(p,tag)
    if      (tag.name == "_group")
      id = QtPersistent::extract_i(p);
    else if (tag.name == "_interval")
      _interval->setText(QString::number(QtPersistent::extract_i(p)));
  XML_iterate_close(QAggSelect,tag);

  _group->button(id)->setChecked(true);
  update_interval();
}

