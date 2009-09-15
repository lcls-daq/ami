#ifndef Pds_ENTRY_HH
#define Pds_ENTRY_HH

class iovec;

namespace Pds {
  class ClockTime;
};

namespace Ami {

  class DescEntry;

  class Entry {
  public:
    Entry();
    virtual ~Entry();

    virtual       DescEntry& desc() = 0;
    virtual const DescEntry& desc() const = 0;

    virtual int update() {return 0;}

    void payload(iovec& iov);
    void payload(iovec& iov) const;

    double                last() const;
    const Pds::ClockTime& time() const;
    void                  time(const Pds::ClockTime& t);

    void reset();

  protected:
    void* allocate(unsigned size);

  private:
    unsigned _payloadsize;
    unsigned long long* _payload;
  };
};

#endif

  
