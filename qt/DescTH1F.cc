#include "DescTH1F.hh"

#include <QtGui/QRadioButton>

using namespace Ami::Qt;

DescTH1F::DescTH1F(const char* name) :
  DescBinning("bins"),
  _button(new QRadioButton(name))
{
}

QRadioButton* DescTH1F::button() { return _button; }
