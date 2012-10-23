#include "ami/qt/QtTable.hh"
#include "ami/data/EntryScalar.hh"

#include <QtCore/QString>
#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>

#include <map>

namespace Ami {
  namespace Qt {
    class QtTableDisplay : public QWidget {
    public:
      QtTableDisplay();
      ~QtTableDisplay();
    public:
      void add   (QtTable&);
      void update();
    protected:
      void paintEvent(QPaintEvent*);
    public:
      static QtTableDisplay* instance();
    private:
      bool _size_changed;
    };
  };
};

using namespace Ami::Qt;

QtTable::QtTable(QObject* p) : _entry(0), _cache(0)
{
  _label = new QLabel("-");
  _value = new QLabel("-");

  QPushButton* xButton = new QPushButton("x");
  xButton->setMaximumHeight(12);
  xButton->setMaximumWidth (12);

  QHBoxLayout* l = new QHBoxLayout;
  l->addWidget(xButton);
  l->addSpacing(4);
  l->addWidget(_label);
  l->addStretch(1);
  l->addWidget(_value);
  setLayout(l);

  QtTableDisplay::instance()->add   (*this);

  connect(this, SIGNAL(destroyed()), p, SLOT(remove()));
  connect(xButton, SIGNAL(clicked()), this, SLOT(remove()));
}

QtTable::~QtTable() 
{
  if (_cache)
    delete _cache;
}

void QtTable::entry (const EntryScalar* e)
{
  _entry = e; 
  if (e) {
    _label->setText(e->desc().name());
    if (_cache)
      delete _cache;
    _cache = new EntryScalar(e->desc());
    _cache->setto(*e);
  }
}

void QtTable::update()
{
  if (!_entry)
    return;

  if (_entry->valid()) {
    double n = _entry->entries() - _cache->entries();
    if (n > 0) {
      double v = (_entry->sum() - _cache->sum())/n;
      _value->setText(QString::number(v));
      _cache->setto(*_entry);
      return;
    }
    _cache->setto(*_entry);
  }
  _value->setText("-");
}

void QtTable::remove()
{
  delete this;
  QtTableDisplay::instance()->update();
}
  

static QtTableDisplay* _instance = 0;

QtTableDisplay::QtTableDisplay() :
  QWidget(0)
{
  setAttribute(::Qt::WA_DeleteOnClose, true);
  QVBoxLayout* l = new QVBoxLayout;
  l->setSpacing(0);
  setLayout(l);
  show();
}

QtTableDisplay::~QtTableDisplay()
{
  _instance = 0;
}

void QtTableDisplay::add(QtTable& q)
{
  static_cast<QVBoxLayout*>(layout())->addWidget(&q);
}

void QtTableDisplay::update()
{
  _size_changed = true;
  updateGeometry();
  resize(minimumWidth(),minimumHeight());
}

void QtTableDisplay::paintEvent(QPaintEvent* e)
{
  if (_size_changed) {
    resize(minimumWidth(),minimumHeight());
    _size_changed = false;
  }
  QWidget::paintEvent(e);
}

QtTableDisplay* QtTableDisplay::instance()
{
  if (!_instance)
    _instance = new QtTableDisplay;

  return _instance;
}
