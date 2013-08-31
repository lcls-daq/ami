#ifndef Ami_AbsOperator_HH
#define Ami_AbsOperator_HH

/**
 *  The AbsOperator class is a base class for objects which operate on
 *  an input Entry and produce an output Entry.  The constructor should
 *  create the output Entry.  Objects of this class are transmitted
 *  over the network, which is achieved using the "serialize" member and
 *  a corresponding constructor (derived classes) to reconstruct the object.
 *  The objects are moved across the network, transmitted with the aid of the
 *  serialize member function and received/reconstituted with the OperatorFactory
 *  class.
 *
 *  New operators should be added to the Type enumeration of this class and added
 *  to the OperatorFactory::_extract() method.
 */

#include <stdint.h>

namespace Ami {

  class DescEntry;
  class Entry;

  class AbsOperator {
  public:
    ///  Enumeration of subclasses
    enum Type { Single, Average, 
		XYProjection, RPhiProjection, 
		Reference, EntryMath, BinMath, EdgeFinder, PeakFinder, 
		EnvPlot, PeakFitPlot, FFT, ContourProjection, TdcPlot, 
                XYHistogram, Zoom, EntryRefOp, Variance, CurveFit, MaskImage,
                BlobFinder, RectROI, FIR };
    ///  Title of subclass operations
    static const char* type_str(Type);

    ///  Constructor with subclass type for reconstitution over the network
    AbsOperator(Type t);
    virtual ~AbsOperator();

    ///  Applies subclass operation to the input data and returns a reference to the result
    Entry&       operator ()(const Entry& e) const;
    ///  Returns a reference to the shape of output data
    DescEntry&   output   () const;
    /**
     *   Builds a serial representation of this object at the input memory location and
     *   returns the memory location for the next object in the serialization.
     */
    void*        serialize(void* p) const;
    ///  Accessor returning the subclass type
    Type         type() const;
    ///  Pointer to next operation (if non-zero) in a cascaded set of operations
    AbsOperator* next() const;
    ///  Appends an operation (set) to be applied after this operation
    void         next(AbsOperator* o);
    ///  Operator was constructed without error
    bool         valid() const;
  protected:
    ///  Subclass-specific operation on input/intermediate data
    virtual Entry& _operate  (const Entry&) const = 0;
    ///  Subclass-specific output/intermediate data shape
    virtual DescEntry& _routput          () const = 0;
    ///  Subclass-specific serialization of object data into memory location
    virtual void*  _serialize(void* p     ) const = 0;
    ///  Subclass-specific determination of valid construction
    virtual bool   _valid     ()            const = 0;
  protected:
    /// helper function for serialization
    void _insert (void*& p, const void* b, unsigned size) const;
    /// helper function for deserialization
    void _extract(const char*& p, void* b, unsigned size);
  private:
    uint32_t     _type;
    AbsOperator* _next;
  };
};

#endif
