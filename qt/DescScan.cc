#include "DescScan.hh"
#include "ami/qt/QtPersistent.hh"
#include "ami/qt/FeatureRegistry.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QRadioButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QComboBox>
#include <QtGui/QIntValidator>
#include <QtGui/QLabel>

using namespace Ami::Qt;

DescScan::DescScan(const char* name) :
  QWidget(0), _button(new QRadioButton(name)),
  _bins(new QLineEdit("200"))
{
  _features = new QComboBox;
  _features->addItems(FeatureRegistry::instance().names());

  _bins->setMaximumWidth(60);
  new QIntValidator   (_bins);

  QVBoxLayout* layout1 = new QVBoxLayout;
  { QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(_button);
    layout->addWidget(_features);
    layout->addStretch();
    layout->addWidget(new QLabel("pts"));
    layout->addWidget(_bins);
    layout1->addLayout(layout); }
  setLayout(layout1);

  connect(&FeatureRegistry::instance(), SIGNAL(changed()), this, SLOT(change_features()));
}

QRadioButton* DescScan::button() { return _button; }
QString  DescScan::expr() const { return _features->currentText(); }
QString  DescScan::feature() const { return QString("[%1]").arg(FeatureRegistry::instance().index(_features->currentText())); }
unsigned DescScan::bins() const { return _bins->text().toInt(); }

void DescScan::save(char*& p) const
{
  QtPersistent::insert(p,_features->currentIndex());
  QtPersistent::insert(p,_bins->text());
}

void DescScan::load(const char*& p)
{
  _features->setCurrentIndex(QtPersistent::extract_i(p));
  _bins->setText(QtPersistent::extract_s(p));
}

void DescScan::change_features()
{
  _features->clear();
  _features->addItems(FeatureRegistry::instance().names());
  _features->setCurrentIndex(0);
}
