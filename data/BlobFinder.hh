#ifndef Ami_BlobFinder_hh
#define Ami_BlobFinder_hh

#include "ami/data/AbsOperator.hh"

namespace Ami {

  class Cds;
  class DescEntry;
  class EntryImage;

  //
  //  Blob (cluster) finder
  //
  class BlobFinder : public AbsOperator {
  public:
    BlobFinder(unsigned roi_top,
               unsigned roi_bottom,
               unsigned roi_left,
               unsigned roi_right,
               unsigned threshold,
               unsigned clustersize,
               bool   accumulate);
    BlobFinder(const char*&);
    BlobFinder(const char*&, const DescEntry&);
    ~BlobFinder();
  public:
  private:
    DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return true; }
  private:
    uint16_t          _roi_top;
    uint16_t          _roi_bottom;
    uint16_t          _roi_left;
    uint16_t          _roi_right;
    uint16_t          _threshold;
    uint16_t          _cluster_size;
    uint32_t          _accumulate;
    uint16_t*         _spectrum;
    EntryImage*       _output_entry;
  };

};

#endif
