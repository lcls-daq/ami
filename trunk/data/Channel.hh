#ifndef Ami_Channel_hh
#define Ami_Channel_hh

#include <vector>

namespace Ami {
  class Channel {
  public:
    enum Option { Single, BitMask };
    Channel(unsigned ch);
    Channel(unsigned ch, Option);
  public:
    operator unsigned() const;
    bool is_mask() const;
    std::vector<unsigned> channels() const;
  private:
    enum { MaskShift=8 };
    unsigned _ch;
  };
};

#endif
