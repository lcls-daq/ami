#ifndef AmiQt_Client_hh
#define AmiQt_Client_hh

//=========================================================
//
//  Client for the Analysis and Monitoring Implementation
//
//  Filter configures the server-side filtering
//  Math   configures the server-side operations
//  Control and Scale configure the client-side processing
//
//=========================================================

#include "ami/client/AbsClient.hh"
#include <QtGui/QWidget>

#include "ami/data/Cds.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace Ami {
  class VClientManager;
  class DescEntry;

  namespace Qt {
    class ChannelDefinition;
    class Control;
    class CursorsX;
    class Display;
    class AxisControl;
    class Status;
    class Transform;
    class EdgeFinder;
    class Client : public QWidget, public Ami::AbsClient  {
      Q_OBJECT
    public:
      Client(const Pds::DetInfo&, unsigned);
      ~Client();
    public:
      void managed         (VClientManager&);
      void request_payload ();
      void one_shot        (bool);
    public:
      void connected       ();
      int  configure       (iovec*);
    public: // AbsClient interface
      int  configured      ();
      void discovered      (const DiscoveryRx&);
      void read_description(Ami::Socket&);
      void read_payload    (Ami::Socket&);
      void process         ();
    public slots:
      void update_configuration();
      void _read_description(int);
    signals:
      void description_changed(int);
    private:
      //  monitored detector channel
      Pds::DetInfo _src;
      unsigned     _channel;

      const DescEntry* _input_entry;
      unsigned    _output_signature;
      char*       _request;
      char*       _description;

      enum {NCHANNELS=4};
      ChannelDefinition* _channels[NCHANNELS];
      EdgeFinder*        _edges;
      CursorsX*          _cursors;
      Control*    _control;
      Transform*  _xtransform;
      Display*    _frame;
      Status*     _status;
      AxisControl* _xrange;
      AxisControl* _yrange;

      bool        _one_shot;

      Cds             _cds;
      VClientManager* _manager;
      unsigned        _niovload;
      iovec*          _iovload;
    };
  };
};

#endif      
