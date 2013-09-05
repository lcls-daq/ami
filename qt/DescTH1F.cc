#include "DescTH1F.hh"

#include <QtGui/QRadioButton>

using namespace Ami::Qt;

DescTH1F::DescTH1F(const char* name, bool autoRange) :
  DescBinning("bins", autoRange),
  _button(new QRadioButton(name))
{
}

QRadioButton* DescTH1F::button() { return _button; }
