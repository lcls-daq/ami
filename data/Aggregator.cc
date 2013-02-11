#include "ami/data/Aggregator.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryTH2F.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryScalarRange.hh"
#include "ami/data/EntryScalarDRange.hh"
#include "ami/service/BSocket.hh"

#include <sys/types.h>
#include <sys/socket.h>

//#define DBUG

using namespace Ami;

static const int BufferSize = 0x2000000;

#ifdef DBUG
static const char* State[] = { "Init", "Connected",
                               "Discovering", "Discovered", 
                               "Configured", 
                               "Describing", "Described",
                               "Processing" };
#endif

Aggregator::Aggregator(AbsClient& client) :
  _client          (client),
  _n               (0),
  _remaining       (0),
  _cds             ("Aggregator"),
  _ocds            ("Output"),
  _niovload        (5),
  _iovload         (new iovec[_niovload]),
  _iovdesc         (new iovec[_niovload+1]),
  _buffer          (new BSocket(BufferSize)),
  _state           (Init),
  _latest          (0),
  _current         (0),
  _request         (EntryList::Full)
{
}

Aggregator::~Aggregator()
{
  delete _buffer;
  delete[] _iovdesc;
  delete[] _iovload;
}

//
//  Add another server
//
void Aggregator::connected       () 
{
  _checkState("conn");
  //
  //  Only forward a connection if it is the first, or it
  //    requires a resync of server states
  //
  _n++;
  _state = Connecting;
}

void Aggregator::disconnected    () 
{
  _checkState("dcon");
  //  printf("Agg disconnect %d %p\n",_n,&_client);
  _n--;
  _client.disconnected(); 
}

//
//  Scale down the statistics required from each server
//
int  Aggregator::configure       (iovec* iov) 
{
  _checkState("conf");
  _state = Configured;
  int n = _client.configure(iov);
  // ...
  return n;
}

int  Aggregator::configured      () 
{
  return _client.configured(); 
}

void Aggregator::discovered      (const DiscoveryRx& rx, unsigned id)
{
  _checkState("dcov",id);

  if (_state != Discovering || id > _current) {
    _state = Discovering;
    _remaining = _n-1;
    _current   = id;
    _nsources  = rx.nsources();
  }
  else {
    _remaining--;
    _nsources += rx.nsources();
  }

  if (_remaining == 0) {
    const_cast<DiscoveryRx&>(rx).nsources(_nsources);
    _state = Discovered;
    _client.discovered(rx);
  }
}

//
//  Keep a cache of the description
//
int Aggregator::read_description(Socket& socket, int len, unsigned id) 
{
  _checkState("desc",id);

  int nbytes = 0;
  if (_n == 1) {
    nbytes += _client.read_description(socket,len);
    _state     = Described;
    _remaining = 0;
  }
  else if ( _n > 1 ) {

    if (len > BufferSize) {
      printf("Aggregator::read_description too large to buffer (%d)\n",len);
      return nbytes;
    }

    int size = socket.read(_buffer->data(),len);
    nbytes += size;

    if (_state == Discovering) {
      //      printf("[%p]   ...discovering .. abort\n",this);
      return nbytes;
    }

    if (_state != Describing || id > _current) {
      _current = id;
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
// 	printf("%s[%d]  norm %c  agg %c\n",
// 	       desc->name(),
// 	       desc->signature(),
// 	       desc->isnormalized() ? 't':'f',
// 	       desc->aggregate() ? 't':'f');
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
    else if (id < _current)
      ;
    else if (--_remaining == 0) {
      _state = Described;
    }
  }
  return nbytes;
}

//
//  Add the payload from one server, and pass the results if all servers seen
//
#define CASEADD(type,f)							\
  case DescEntry::type:							\
  reinterpret_cast<Entry##type*>(payload)->f(*reinterpret_cast<Entry##type*>(iovl->iov_base)); \
  break;

#define SETNORM(type,e)							\
  case DescEntry::type:							\
  norm = reinterpret_cast<Entry##type*>(e)->info(Entry##type::Normalization); \
  break;

#include "pdsdata/xtc/ClockTime.hh"

int  Aggregator::read_payload    (Socket& s, int sz, unsigned id) 
{
  _checkState("payl");
  int nbytes = 0;
  if (_state != Described) {
    //    printf("[%p] Agg read_payload state %s\n", this, State[_state]);
    return nbytes;
  }

  if ( _n == 1 ) { 
    nbytes =_client.read_payload(s,sz);
    _remaining = 0;
  }
  else if ( _n > 1 ) {

    if (_remaining==0 || id > _current) {  // simply copy
      _current = id;
      nbytes = s.read(_buffer->data(),sz);
      _remaining = _n-1;
    }
    else {  // aggregate
      int niov = _cds.payload(_iovload,_request);
      _cds.description(_iovdesc,_request);
      nbytes = s.readv(_iovload,niov);
      iovec* iovl = _iovload;
      iovec* iovd = _iovdesc+1;
      char* payload = _buffer->data();
      while(niov--) {
	DescEntry* desc = reinterpret_cast<DescEntry*>(iovd->iov_base);
	_cds.entry(desc->signature())->merge(payload);
	payload += iovl->iov_len;
	iovl++;
	iovd++;
      }
      if (--_remaining == 0) {
	_client.read_payload(*_buffer,sz);
      }
    }
  }
  return nbytes;
}

bool Aggregator::svc() const { return _client.svc(); }

//
//  If all servers seen, process.
//
void Aggregator::process         () 
{
  _checkState("proc");
  if (_state==Described && _remaining==0)
    _client.process();
}

void Aggregator::tmo()
{
  _checkState("tmo");
  if (_state == Connected)    
    _client.connected();
  else if (_state == Connecting)    
    _state = Connected;
}

void Aggregator::_checkState(const char* s)
{
#ifdef DBUG
  printf("[%p] : %s : State %s : remaining %d/%d\n", this, s, State[_state], _remaining, _n);
#endif
}

void Aggregator::_checkState(const char* s, unsigned id)
{
#ifdef DBUG
  printf("[%p] : %s : State %s : id %d/%d: remaining %d/%d\n", 
         this, s, State[_state], id, _current, _remaining, _n);
#endif
}

void Aggregator::request_payload(const EntryList& request) 
{
  _request = request; 
}
