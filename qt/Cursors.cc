#include "Cursors.hh"

#include "ami/qt/CursorTarget.hh"

using namespace Ami::Qt;

Cursors::Cursors(CursorTarget& frame) : _frame(frame) {}
  
Cursors::~Cursors() {}

void Cursors::grab_cursor() { _frame.set_cursor_input(this); }

void Cursors::set_cursor(double x, double y)
{
  _frame.set_cursor_input(0);
  _set_cursor(x,y);
}

