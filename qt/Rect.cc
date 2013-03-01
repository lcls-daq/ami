#include "ami/qt/Rect.hh"
#include "ami/qt/QtPersistent.hh"

using namespace Ami::Qt;

void Rect::save(char*& p) const
{
  XML_insert(p, "int", "x0", QtPersistent::insert(p,x0) );
  XML_insert(p, "int", "y0", QtPersistent::insert(p,y0) );
  XML_insert(p, "int", "x1", QtPersistent::insert(p,x1) );
  XML_insert(p, "int", "y1", QtPersistent::insert(p,y1) );
}

void Rect::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.name == "x0")
      x0 = QtPersistent::extract_i(p);
    else if (tag.name == "y0")
      y0 = QtPersistent::extract_i(p);
    else if (tag.name == "x1")
      x1 = QtPersistent::extract_i(p);
    else if (tag.name == "y1")
      y1 = QtPersistent::extract_i(p);
  XML_iterate_close(Rect,tag);
}
