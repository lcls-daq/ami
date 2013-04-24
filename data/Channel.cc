#include "ami/data/Channel.hh"

using namespace Ami;

Channel::Channel(unsigned ch) : _ch(ch) {}

Channel::Channel(unsigned ch, Channel::Option o) :
  _ch ( o==BitMask ? (ch<<MaskShift) : (ch & ((1<<MaskShift)-1)) ) {}

Channel::operator unsigned() const { return _ch; }

bool Channel::is_mask() const { return (_ch>>MaskShift)!=0; }

std::vector<unsigned> Channel::channels() const 
{
  std::vector<unsigned> n(0);
  unsigned ch = _ch>>MaskShift;
  for(unsigned i=0; ch!=0; i++) {
    if (ch&1)
      n.push_back(i);
    ch >>= 1;
  }
  return n;
}
