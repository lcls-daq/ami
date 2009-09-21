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

class QLayout;

namespace Ami {
  class VClientManager;
  class DescEntry;
  class Semaphore;

  namespace Qt {
    class ChannelDefinition;
    class Control;
    class Display;
    class AxisControl;
    class Status;
    class Transform;

    class Client : public QWidget, public Ami::AbsClient  {
      Q_OBJECT
    public:
      Client(const Pds::DetInfo&, unsigned, Display*);
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
    protected:
      void              addWidget(QWidget*);
      Ami::Qt::Display& display  ();
    private:
      virtual void _configure(char*& p, 
			      unsigned input, 
			      unsigned& output,
			      ChannelDefinition* ch[], 
			      int* signatures, 
			      unsigned nchannels) {}
      virtual void _setup_payload(Cds&) {}
      virtual void _update() {}
    private:
      //  monitored detector channel
      Pds::DetInfo _src;
      unsigned     _channel;

    protected:
      enum {NCHANNELS=4};
      ChannelDefinition* _channels[NCHANNELS];
      Display*           _frame;
      const DescEntry*   _input_entry;

    private:
      unsigned    _output_signature;
      char*       _request;
      char*       _description;

      Control*    _control;
      Transform*  _xtransform;
      Status*     _status;
      AxisControl* _xrange;
      AxisControl* _yrange;

      bool        _one_shot;

      Cds             _cds;
      VClientManager* _manager;
      unsigned        _niovload;
      iovec*          _iovload;

      QLayout*    _layout;

      Semaphore*  _sem;
    };
  };
};

#endif      
