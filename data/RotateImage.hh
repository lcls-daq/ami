#ifndef Ami_RotateImage_HH
#define Ami_RotateImage_HH

#include "ami/data/AbsOperator.hh"
#include "ami/data/DescImage.hh"

namespace Ami {

  class RotateImage : public AbsOperator {
  public:
    ///  Constructor with subclass type for reconstitution over the network
    RotateImage(const DescEntry& output, Ami::Rotation);
    RotateImage(const char*&, const DescEntry&);
    RotateImage(const char*&);
    virtual ~RotateImage();

  protected:
    Entry& _operate  (const Entry&) const;
    const DescEntry& _routput    () const;
    void*  _serialize(void* p     ) const;
    bool   _valid     () const { return true; }
    void   _invalid   ();
  private:
    enum { DESC_LEN = sizeof(DescImage) };
    uint32_t     _desc_buffer[DESC_LEN/sizeof(uint32_t)];
    Rotation     _rotation;
    Entry*       _output;
    
  };
};

#endif
