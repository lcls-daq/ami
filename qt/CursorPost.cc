#include "ami/qt/CursorPost.hh"

#include "ami/qt/CPostParent.hh"

#include "ami/data/BinMath.hh"
#include "ami/data/Cds.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/QtPersistent.hh"

#include <QtCore/QString>

#include <stdio.h>

using namespace Ami::Qt;

CursorPost::CursorPost(unsigned         channel,
		       BinMath*         input,
                       CPostParent*     parent) :
  _parent  (parent)
{
  _channel = channel;
  _input   = input;
}

CursorPost::CursorPost(const char*& p) : 
  _parent (0)
{
  _channel = 0;
  _input   = 0;
  load(p);
}

CursorPost::~CursorPost()
{
  if (_parent)
    _parent->remove_cursor_post(this); 
  if (_input ) 
    delete _input;
}

void CursorPost::save(char*& p) const
{
  char* buff = new char[8*1024];
  XML_insert(p, "int"    , "_channel", QtPersistent::insert(p,(int)_channel) );
  XML_insert(p, "BinMath", "_input"  , QtPersistent::insert(p,buff,(char*)_input->serialize(buff)-buff) );
  delete[] buff;
}

void CursorPost::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.name == "_channel")
      _channel = QtPersistent::extract_i(p);
    else if (tag.name == "_input") {
      if (_input) delete _input;
      const char* b = (const char*)QtPersistent::extract_op(p);
      b += 2*sizeof(uint32_t);
      _input = new BinMath(b);
    }
  XML_iterate_close(CursorPost,tag);
}

