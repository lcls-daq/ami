#ifndef Ami_AbsFilter_hh
#define Ami_AbsFilter_hh

/**
 *  The AbsFilter class is a base class for a decision upon whether to apply
 *  an analysis (operators) to the input data.  The object is either a simple
 *  pass through (Raw), a selection based upon a scalar variable falling within 
 *  a defined range, or a logical combination of those.
 *  
 *  The objects are moved across the network, transmitted with the aid of the
 *  serialize member function and received/reconstituted with the FilterFactory
 *  class.
 */

#include <stdint.h>
#include <string>

namespace Ami {
  class AbsFilter {
  public:
    ///  Enumerated subclasses
    enum Type { Raw,      // simple filters
		SingleShot,
		FeatureRange,
		LogicAnd, // compound filters
		LogicOr };
    AbsFilter(Type t) : _type(t) {}
    virtual ~AbsFilter() {}
  public:
    Type  type() const { return (Type)_type; }
  public:
    ///  Mark the inputs used by this operation
    virtual void use   () const {}
    ///  Returns status of whether inputs are complete
    virtual bool  valid() const = 0;
    ///  Returns decision of whether to process the event
    virtual bool  accept() const = 0;
    ///  Clones filter object for application to another analysis thread
    virtual AbsFilter* clone() const = 0;
    ///  Converts to a readable expression
    virtual std::string text() const = 0;
  public:
    ///  Serializes object data for network transmission.
    void* serialize(void*) const;
  private:
    virtual void* _serialize(void* p) const = 0;
  protected:
    //  helper functions for (de)serialization
    void _insert (void*& p, const void* b, unsigned size) const;
    void _extract(const char*& p, void* b, unsigned size);
  private:
    uint32_t _type;
  };
};

#endif
