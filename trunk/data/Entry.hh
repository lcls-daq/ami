#ifndef Pds_ENTRY_HH
#define Pds_ENTRY_HH

class iovec;

namespace Pds {
  class ClockTime;
};

namespace Ami {

  class DescEntry;

  /**
   *   @brief Base class for representing plottable data
   *   @autho Matt Weaver
   *
   *   The Entry class references plottable data.  It maintains accessors to
   *   describe the shape of the data, the last time of update, and the payload itself.
   *   It also holds methods for network transmission and reception / merging of datasets.
   * 
   */
  class Entry {
  public:
    Entry();
    virtual ~Entry();

    /**
     *  Returns a description of the shape of the data {dimensions,binning,titles}
     *  necessary for interpreting the payload upon reception.
     */
    virtual       DescEntry& desc() = 0;
    virtual const DescEntry& desc() const = 0;

    ///  Accessor to the raw data itself
    const unsigned long long* payload() const { return _payload; }

    ///  Fills the iovec structure with the location and size of memory for transmission
    void payload(iovec& iov) const;

    /**
     *   Accessors for determining time of most recent valid data.
     */
    ///  Returns true if the payload contains data from an update
    bool                  valid  () const;
    ///  Returns most recent update time in seconds
    double                last() const;
    ///  Returns first update time in seconds
    double                first() const;
    ///  Returns most recent update time in wall-clock {seconds,nanoseconds}
    const Pds::ClockTime& time() const;

    ///  Modifiers for setting time of most recent valid data
    void                  valid  (const Pds::ClockTime& t);
    void                  invalid();

    ///  Zeroes payload contents and time of last update
    void reset();

    ///  Merges payload with input data.  Result is stored in input data.
    void merge(char*) const;

  protected:
    ///  Memory allocation for payload
    void* allocate(unsigned size);

  private:
    ///  Helper for data shape specific merge function
    virtual void _merge (char*) const {}

    ///  Payload size in bytes
    unsigned _payloadsize;
    ///  Pointer to payload beginning (time of update stored first)
    unsigned long long* _payload;

    friend class EntryView;
  };
};

#endif

  
