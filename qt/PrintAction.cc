#include "PrintAction.hh"

#include <QtGui/QPrinter>

using namespace Ami::Qt;

PrintAction::PrintAction(QWidget& d) :
  QAction("Print",&d),
  _display(d)
{
}

PrintAction::~PrintAction() {}

void PrintAction::triggered()
{
  _display.render(_printer);
}

QPrinter* PrintAction::printer()
{
  if (!_printer)
    _printer = new QPrinter;
  return _printer;
}

QPrinter* PrintAction::_printer = 0;
