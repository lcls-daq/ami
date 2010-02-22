#include "ami/data/Aggregator.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/service/BSocket.hh"

#include <sys/types.h>
#include <sys/socket.h>

using namespace Ami;

static const int BufferSize = 0x800000;

Aggregator::Aggregator(AbsClient& client) :
  _client          (client),
  _n               (0),
  _cds             ("Aggregator"),
  _niovload        (5),
  _iovload         (new iovec[_niovload]),
  _iovdesc         (new iovec[_niovload+1]),
  _buffer          (new BSocket(BufferSize)),
  _state           (Init)
{
}

Aggregator::~Aggregator()
{
}

//
//  Add another server
//
void Aggregator::connected       () { _n++; }

//
//  Scale down the statistics required from each server
//
int  Aggregator::configure       (iovec* iov) 
{
  _state = Configured;
  int n = _client.configure(iov);
  // ...
  return n;
}

int  Aggregator::configured      () 
{
  return _client.configured(); 
}

void Aggregator::discovered      (const DiscoveryRx& rx)
{
  _client.discovered(rx); 
}

//
//  Keep a cache of the description
//
void Aggregator::read_description(Socket& socket, int len) 
{
  if ( _n > 1 ) {

    if (len > BufferSize) {
      printf("Aggregator::read_description too large to buffer (%d)\n",len);
      return;
    }

    int size = socket.read(_buffer->data(),len);

    if (_state != Describing) {
      _cds.reset();
    
      const char* payload = _buffer->data();
      const char* const end = payload + size;
      payload += sizeof(Desc);
      
      while( payload < end ) {
	const DescEntry* desc = reinterpret_cast<const DescEntry*>(payload);
	if (desc->size()==0) {
	  printf("read_description size==0\n");
	  break;
	}
	Entry* entry = EntryFactory::entry(*desc);
	_cds.add(entry, desc->signature());
	payload += desc->size();
      }

      if (_cds.totalentries()>_niovload) {
	delete[] _iovload;
	_iovload = new iovec[_niovload=_cds.totalentries()];
	delete[] _iovdesc;
	_iovdesc = new iovec[_cds.totaldescs()];
      }
      _cds.payload    (_iovload);
      _cds.description(_iovdesc);

      _client.read_description(*_buffer,len);
      _remaining = _n-1;
      _state = Describing;
    }
    else if (--_remaining == 0) {
      _state = Described;
    }
  }
  else {
    _client.read_description(socket,len);
    _state = Described;
  }
}

//
//  Add the payload from one server, and pass the results if all servers seen
//
#define CASEADD(type,f)							\
  case DescEntry::type:							\
  reinterpret_cast<Entry##type*>(payload)->f(*reinterpret_cast<Entry##type*>(iovl->iov_base)); \
  break;


void Aggregator::read_payload    (Socket& s, int niov) 
{
  if ( _n > 1 ) {
    s.readv(_iovload,niov);
    if (_state == Processed) {  // simply copy
      iovec* iov = _iovload;
      char* payload = _buffer->data();
      while(niov--) {
	memcpy(payload, iov->iov_base, iov->iov_len);
	payload += iov->iov_len;
	iov++;
      }
      _remaining = _n-1;
    }
    else {  // aggregate
      iovec* iovl = _iovload;
      iovec* iovd = _iovdesc+1;
      char* payload = _buffer->data();
      while(niov--) {
	DescEntry* desc = reinterpret_cast<DescEntry*>(iovd->iov_base);
	switch(desc->type()) {
	case DescEntry::Scalar:
	case DescEntry::TH1F:
	case DescEntry::Waveform:
	case DescEntry::Prof:
	case DescEntry::Image:
	  { const char* base = (const char*)iovl->iov_base;
	    double* dst = reinterpret_cast<double*>(payload);
	    const double* src = reinterpret_cast<const double*>(base);
	    const double* end = reinterpret_cast<const double*>(base+iovl->iov_len);
	    do {
	      *dst++ += *src++;
	    } while (src < end);
	  } break;
	case DescEntry::Scan:  // This one's hard
	  //  Consider scans that don't gather enough events to see all servers
	  //  Consider bld that is different for event event
	  break;
	case DescEntry::TH2F:
	default:
	  break;
	}
	payload += iovl->iov_len;
	iovl++;
	iovd++;
      }
      if (--_remaining==0)
	_state = Processing;
    }
  }
  else {
    _client.read_payload(s,niov);
    _state = Processing;
  }
}

//
//  If all servers seen, process.
//
void Aggregator::process         () 
{
  if (_state != Processing)
    return;

  _client.process();
  _state = Processed;
}
