#ifndef Ami_RefreshRequest_hh
#define Ami_RefreshRequest_hh

#include "ami/data/ConfigureRequest.hh"
#include "pdsdata/xtc/ClockTime.hh"

namespace Ami {

  class RefreshRequest {
  public:
    RefreshRequest(ConfigureRequest::Source source,
                   int                      input,
                   const Pds::ClockTime&    tmo) :
      _source(source),
      _input (input),
      _tmo   (tmo)
    {
    }
  public:
    ConfigureRequest::Source source() const { return ConfigureRequest::Source(_source); }
    int    input () const { return _input; }
    const Pds::ClockTime& tmo() const { return _tmo; }
  private:
    uint32_t _source;
    int32_t  _input;
    Pds::ClockTime _tmo;
  };
};

#endif
