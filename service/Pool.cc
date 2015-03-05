#include "ami/service/Pool.hh"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace Ami;

Pool::Pool(int sz) :
  _buffer (new char[sz]),
  _current(_buffer),
  _size   (sz)
{}

Pool::~Pool()
{ delete[] _buffer; }

char* Pool::extend(int sz)
{
  _current += sz;

  if (_current > _buffer + _size) {  // overflow
    printf("Ami::Pool buffer overflow [%zd/%d]\n",_current-_buffer,_size);
    abort();
  }
  else if (_current + (_size>>2) > _buffer + _size)
    _resize( _size << 1 );

  return _current;
}

void Pool::reserve(int sz)
{
  if (_current + sz > _buffer + _size) 
    _resize( sz );
}

void Pool::_resize(int size)
{
  char* p  = new char[size];
  int csz  = _current-_buffer;

  memcpy(p, _buffer, csz);
  delete[] _buffer;
  
  _buffer  = p;
  _current = p + csz;
  _size    = size;
}
