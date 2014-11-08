#include "ami/qt/EnvPost.hh"

#include "ami/data/QtPersistent.hh"

#include <stdio.h>

using namespace Ami::Qt;

EnvPost::EnvPost(const Ami::AbsFilter& filter,
                 DescEntry*            desc,
                 Ami::ScalarSet        set) :
  EnvOp(filter,desc,set)
{
}

EnvPost::EnvPost(const char*& p)
{
  XML_iterate_open(p,tag)
    if (EnvOp::load(tag,p))
      ;
  XML_iterate_close(EnvPost,tag);
}

EnvPost::~EnvPost()
{
}
