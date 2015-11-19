#include "ami/qt/Rotator.hh"
#include "ami/qt/Client.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/RawFilter.hh"
#include "ami/data/RotateImage.hh"

#include <QtGui/QComboBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QWidget>
#include <QtGui/QLabel>
#include <QtCore/QString>

using namespace Ami::Qt;

static Ami::RawFilter _noFilter;

Rotator::Rotator(Client& client) :
  _prototype("image",1,1),
  _op       (0)
{
  QChar degree(0x00B0);
  _roBox = new QComboBox;
  _roBox->addItem(QString("  0"));
  _roBox->addItem(QString(" 90%1").arg(degree));
  _roBox->addItem(QString("180%1").arg(degree));
  _roBox->addItem(QString("270%1").arg(degree));
  QWidget* w = new QWidget;
  QHBoxLayout* hl = new QHBoxLayout;
  hl->addWidget(_roBox);
  hl->addWidget(new QLabel("Rotate\nDisplay"));
  w->setLayout(hl);
  _widget=w;
}

Rotator::~Rotator()
{
  if (_op) delete _op;
}

QWidget* Rotator::widget   () const { return _widget; }

QComboBox* Rotator::box    () const { return _roBox; }

void     Rotator::save(char*& p) const
{
  XML_insert(p, "QComboBox", "_roBox", QtPersistent::insert(p,_roBox->currentIndex()) );
}

void     Rotator::load(const char*& p) 
{
  XML_iterate_open(p,tag)
    if (tag.element == "_roBox")
      _roBox->setCurrentIndex(QtPersistent::extract_i(p));
  XML_iterate_close(PnccdClient,tag);
}

void     Rotator::prototype(const DescEntry& d)
{
  _prototype = static_cast<const DescImage&>(d);
  printf("Rotator prototype %d\n",d.signature());
}

unsigned Rotator::configure(char*&    p,
                            unsigned  input,
                            unsigned& output,
                            ConfigureRequest::Source& source)
{
  printf("Rotator configure input %d rotation %d\n",
         input, unsigned(rotation()));

  if (rotation()==Ami::D0)
    return input;

  if (_op) delete _op;
  _op = new RotateImage(_prototype,rotation());
  ConfigureRequest& r = *new(p) ConfigureRequest(ConfigureRequest::Create,
                                                 source,
                                                 input,
                                                 -1,
                                                 _noFilter,
                                                 *_op);

  source = ConfigureRequest::Analysis;
  p += r.size();
  _req.request(r, output, false);

  printf("Rotator configure output %d\n",r.output());

  return r.output();
}

Ami::Rotation Rotator::rotation() const { return Rotation(_roBox->currentIndex()); }
