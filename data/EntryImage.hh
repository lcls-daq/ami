#ifndef Pds_ENTRYImage_HH
#define Pds_ENTRYImage_HH

#include "ami/data/Entry.hh"
#include "ami/data/DescImage.hh"
#include "ndarray/ndarray.h"

namespace Ami {

  class EntryImage : public Entry {
  public:
    EntryImage(const Pds::DetInfo&, unsigned channel, const char* name);
    EntryImage(const DescImage& desc);

    virtual ~EntryImage();

    void params(unsigned nbinsx,
		unsigned nbinsy,
		int ppxbin,
		int ppybin);
    void params(const DescImage& desc);

    /*
    **  Access the contents by bin index
    */
    unsigned                        content(unsigned bin) const;
    unsigned                        content(unsigned binx, unsigned biny) const;

    /*
    **  Access the contents array
    */
    uint32_t*                       contents();
    const uint32_t*                 contents() const;

    /*
    **  Access the contents by ndarray
    */
    ndarray<uint32_t,2>             content();
    const ndarray<const uint32_t,2> content() const;
    /*
     *  Access the SubFrame contents by ndarray
     */
    ndarray<uint32_t,2>             contents(unsigned);
    const ndarray<const uint32_t,2> contents(unsigned) const;
    
    ndarray<uint32_t,2>             contents(const SubFrame&);
    const ndarray<const uint32_t,2> contents(const SubFrame&) const;

    /*
     *  Modify contents
     */
    void                            addcontent(unsigned y, unsigned binx, unsigned biny);
    void                            content   (unsigned y, unsigned binx, unsigned biny);

    void content(const ndarray<const uint16_t,2>&);
    void content(const ndarray<const int16_t,2>&);
    void content(const ndarray<const uint8_t,2>&);

    enum Info { Pedestal, Normalization, InfoSize };
    double   info   (Info) const;
    void     info   (double y, Info);
    void     addinfo(double y, Info);

    void setto(const EntryImage& entry);
    void setto(const EntryImage& curr, const EntryImage& prev);
    void add  (const EntryImage& entry);

    // Implements Entry
    virtual const DescImage& desc() const;
    virtual DescImage& desc();

  private:
    virtual void _merge(char*) const;

  private:
    void build();

  private:
    DescImage _desc;

  private:
    uint32_t* _y;
  };

  inline unsigned EntryImage::content(unsigned bin) const 
  {
    return *(_y+bin); 
  }
  inline unsigned EntryImage::content(unsigned binx, unsigned biny) const 
  {
    return *(_y+binx+biny*_desc.nbinsx()); 
  }
  inline void EntryImage::addcontent(unsigned y, unsigned binx, unsigned biny) 
  {
    *(_y+binx+biny*_desc.nbinsx()) += y;
  }
  inline void EntryImage::content(unsigned y, unsigned binx, unsigned biny) 
  {
    *(_y+binx+biny*_desc.nbinsx()) = y;
  }
  inline uint32_t* EntryImage::contents()
  {
    return _y;
  }
  inline const uint32_t* EntryImage::contents() const
  {
    return _y;
  }
  inline double EntryImage::info(Info i) const 
  {
    return reinterpret_cast<float*>(_y+_desc.nbinsx()*_desc.nbinsy())[i];
  }
  inline void EntryImage::info(double y, Info i) 
  {
    reinterpret_cast<float*>(_y+_desc.nbinsx()*_desc.nbinsy())[i] = y;
  }
  inline void EntryImage::addinfo(double y, Info i) 
  {
    reinterpret_cast<float*>(_y+_desc.nbinsx()*_desc.nbinsy())[i] += y;
  }
  inline ndarray<uint32_t,2> EntryImage::content()
  {
    return make_ndarray(_y,_desc.nbinsy(),_desc.nbinsx());
  }
  inline const ndarray<const uint32_t,2> EntryImage::content() const
  {
    return make_ndarray(_y,_desc.nbinsy(),_desc.nbinsx());
  }
};

#endif
