#ifndef Ami_EpicsCA_hh
#define Ami_EpicsCA_hh

//--------------------------------------------------------------------------
//
// File and Version Information:
// 	$Id: EpicsCA.hh 1240 2011-01-05 19:43:28Z weaver $
//
// Environment:
//      This software was developed for the BaBar collaboration.  If you
//      use all or part of it, please give an appropriate acknowledgement.
//      This class was copied from OdcCompProxy/EpicsCAProxy for use
//      in the online environment.
//
//
//------------------------------------------------------------------------

#include "PVMonitorCb.hh"

// epics includes
#include "cadef.h"

namespace Ami_Epics {

  class EpicsCA;

  //==============================================================================
  class EpicsCAChannel {
  public:
    enum ConnStatus { NotConnected, Connecting, Connected };

    EpicsCAChannel(const char* channelName,
		   bool        monitor,
		   EpicsCA&    proxy);
    ~EpicsCAChannel();
    
    void connect    (void);
    void get        ();
    void put        ();
    void put_cb     ();
  public:
    void getDataCallback   (struct event_handler_args ehArgs);
    void putDataCallback   (struct event_handler_args ehArgs);
    void connStatusCallback(struct connection_handler_args chArgs);
    const char* epicsName  (void) { return _epicsName; }
    chtype     type        () const { return _type; }
    int        nelements   () const { return _nelements; }
    ConnStatus connected   () const { return _connected; }
  
  protected:
    char        _epicsName[32];
    int         _nelements;
    chid	_epicsChanID;
    chtype      _type;
    evid        _event;
    ConnStatus  _connected;
    bool        _monitor;
    EpicsCA&    _proxy;
  };

  //==============================================================================
  class EpicsCA {
  public:
    EpicsCA(const char *channelName, PVMonitorCb*);
    virtual ~EpicsCA();
  public:  
    virtual void  connected(bool);
    virtual void  getData  (const void* value);
    virtual void  putStatus(bool);
  public:
    void*  data     ();
    size_t data_size() const;
    bool   connected() const;
  protected:
    EpicsCAChannel   _channel;
    PVMonitorCb*     _monitor;
    char* _pvdata;
    int   _pvsiz;
    bool  _connected;
  };
};

#endif
