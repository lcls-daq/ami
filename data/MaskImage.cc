#include "MaskImage.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryFactory.hh"

#include <string.h>
#include <stdio.h>

using namespace Ami;

MaskImage::MaskImage(const char* path) :
  AbsOperator(AbsOperator::MaskImage),
  _output    (0)
{
  if (path)
    strncpy(_path,path,PATH_LEN);
  else
    memset(_path,0,PATH_LEN);
}

MaskImage::MaskImage(const char*& p, const DescEntry& e) :
  AbsOperator(AbsOperator::MaskImage)
{
  _extract(p,_path, PATH_LEN);

  DescImage desc(*static_cast<const DescImage*>(&e), _path);

  _output = EntryFactory::entry(desc);
}

MaskImage::~MaskImage()
{
  if (_output) delete _output;
}

DescEntry& MaskImage::_routput   () const { return _output->desc(); }

void*      MaskImage::_serialize(void* p) const
{
  _insert(p, _path , PATH_LEN);
  return p;
}

Entry&     MaskImage::_operate(const Entry& e) const
{
  if (e.valid()) {
    EntryImage& entry = *static_cast<EntryImage*>(_output);
    entry.setto(static_cast<const EntryImage&>(e));
  }

  return *_output;
}

