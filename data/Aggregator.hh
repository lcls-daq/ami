#ifndef Ami_Aggregator_hh
#define Ami_Aggregator_hh

#include "ami/client/AbsClient.hh"
#include "ami/data/Cds.hh"
#include "ami/data/EntryList.hh"

#include <string>

class iovec;

namespace Ami {

  class BSocket;

  class Aggregator {
  public:
    Aggregator(AbsClient&);
    ~Aggregator();
  public:
    void connected       () ;
    void disconnected    () ;
    int  configure       (iovec*) ;
    int  configured      () ;
    void discovered      (const DiscoveryRx&,unsigned) ;
    int  read_description(Socket&,int,unsigned) ;
    int  read_payload    (Socket&,int,unsigned) ;
    bool svc             () const;
    void process         () ;
    void tmo             () ;
  public:
    void request_payload (const EntryList&);
    void dump_throttle   () const;
  public:
    std::string dump     () const;
  private:
    void _checkState     (const char*);
    void _checkState     (const char*, unsigned);
  private:
    AbsClient& _client;
    uint64_t   _allocated;
    unsigned   _n;
    unsigned   _remaining;
    unsigned   _nsources;
    Cds        _cds;
    Cds        _ocds;
    unsigned   _niovload;
    iovec*     _iovload;
    iovec*     _iovdesc;
    BSocket*   _buffer;
    enum { Init, Connecting, Connected, 
           Discovering, Discovered, Configured, 
           Describing, Described, Processing } _state;
    unsigned   _latest;
    unsigned   _current;
    unsigned   _nprocess;
    unsigned   _tmo;
    unsigned   _tag;
    EntryList  _request;
  };
};

#endif
