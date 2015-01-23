#include "Reference.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/valgnd.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include <sys/uio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <vector>

using namespace Ami;

#define READ_SIZE(buffer,len,f) {                                       \
    size = fread(buffer,1,len,f);                                       \
    printf("reference read %d bytes\n",int(size));                      \
    if ((unsigned)size != len) {                                        \
      printf("reference read %d/%d bytes\n",int(size),int(len));        \
      abort();                                                          \
    }                                                                   \
  }

Reference::Reference(const char* path) : 
  AbsOperator(AbsOperator::Reference),
  _entry     (0)
{
  strncpy_val(_path,path,PATHLEN);
}

Reference::Reference(const char*& p, const DescEntry& e) :
  AbsOperator(AbsOperator::Reference)
{
  _extract(p, _path, PATHLEN);
  FILE* f = fopen(_path,"r");
  if (!f) {
    printf("reference failed to open %s\n",_path);
    abort();
  }
  int size, i;
  /*
   * Check if this is text or a DescEntry.
   *
   * Scan the file.  If everything is printable or whitespace,
   * assume it is text.
   */
  while ((i = fgetc(f)) != EOF)
      if (!isprint(i) && !isspace(i))
          break;
  rewind(f);
  if (i == EOF) { // Looks like text!
      double tf = 0.0, tl = 0.0;
      std::vector<double> _data;
      double tt, dd;

      _data.clear();
      i = 0;
      while (fscanf(f, "%lg %lg\n", &tt, &dd) == 2) {
          if (!i)
              tf = tt;
          else
              tl = tt;
          _data.push_back(dd);
          i++;
      }
      fclose(f);
      Pds::DetInfo info(0,Pds::DetInfo::NoDetector,0,Pds::DetInfo::NoDevice,0);
      DescWaveform *dwf =
          new (_buffer) DescWaveform(info, -1, "Reference", "xtitle", "ytitle", i, tf, tl);
      dwf->fix(true);
      _entry = EntryFactory::entry(*dwf);
      while (--i >= 0) {
          ((EntryWaveform *)_entry)->content(_data[i], i);
      }
      _entry->valid(Pds::ClockTime(0,0)); // Mark it valid with a bogus time!
      return;
  }
  READ_SIZE(_buffer,sizeof(DescEntry),f);
  const DescEntry& d = *reinterpret_cast<const DescEntry*>(_buffer);
  if (d.size()>sizeof(_buffer)) {
    printf("reference attempt to read desc size %d from %s\n",d.size(),_path);
    abort();
  }
  READ_SIZE(_buffer+sizeof(DescEntry)/sizeof(uint32_t),d.size()-sizeof(DescEntry),f);
  _entry = EntryFactory::entry(d);
  iovec iov;
  _entry->payload(iov);
  READ_SIZE(iov.iov_base,iov.iov_len,f);
  fclose(f);
  _entry->desc().fix(true);
}

Reference::~Reference()
{
  if (_entry) delete _entry;
}

const DescEntry& Reference::_routput   () const { return _entry->desc(); }

void*      Reference::_serialize(void* p) const
{
  _insert(p, _path, PATHLEN);
  return p;
}

Entry&     Reference::_operate(const Entry& e) const
{
  _entry->valid(e.time());
  return *_entry;
}
