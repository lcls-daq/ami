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

#include "ami/qt/AbsClient.hh"

#include "ami/qt/QtPStack.hh"
#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/service/Pool.hh"

#include <QtGui/QHBoxLayout>

class QLayout;
class QVBoxLayout;

namespace Ami {
  class ClientManager;
  class DescEntry;
  class Semaphore;

  namespace Qt {
    class AggChannels;
    class ChannelDefinition;
    class Control;
    class Display;
    class Status;

    class Client : public Ami::Qt::AbsClient {
      Q_OBJECT
    public:
      Client(QWidget*,const Pds::DetInfo&, unsigned, const QString&,
	     Display*, 
	     double request_rate=0);
      ~Client();
    public:
      const QString& title() const;
      void save(char*& p) const;
      void load(const char*& p);
      void reset_plots();
    public:
      void managed         (ClientManager&);
      void request_payload ();
      void one_shot        (bool);
    public:
      void connected       ();
      int  configure       (iovec*);
    public: // AbsClient interface
      int  configured      ();
      void discovered      (const DiscoveryRx&);
      int  read_description(Ami::Socket&,int);
      int  read_payload    (Ami::Socket&,int);
      bool svc             () const;
      void process         ();
      QWidget* window()  { return _layout->parentWidget()->window(); }
    public slots:
      void update_configuration();
      void update_configuration(bool);
      void _read_description(int);
    signals:
      void description_changed(int);
    protected:
      void              addWidget (QWidget*);
      void              addOverlay(QWidget*);
      Ami::Qt::Display& display  ();
      const Ami::Qt::Display& display  () const;
    protected:
      virtual unsigned _preconfigure(char*&    p,
                                     unsigned  input,
                                     unsigned& output,
                                     ConfigureRequest::Source&);
      virtual void _configure(char*& p, 
			      unsigned input, 
			      unsigned& output,
			      ChannelDefinition* ch[], 
			      int* signatures, 
			      unsigned nchannels) {}
      virtual void _setup_payload(Cds&) {}
      virtual void _update() {}
      virtual void _prototype(const DescEntry&) {}

      virtual void showEvent(QShowEvent*);
      virtual void hideEvent(QHideEvent*);
    protected:
      enum {NCHANNELS=4};
      ChannelDefinition* _channels[NCHANNELS];
      AggChannels*       _agg;
      Display*           _frame;
      const DescEntry*   _input_entry;
      unsigned           _input;
      ConfigureRequest::Source _input_source;

    private:
      QString     _title;
      unsigned    _output_signature;
      Pool        _request;
      Pool        _description;

    protected:
      Control*    _control;
      Status*     _status;

      bool        _one_shot;

    protected:
      Cds             _cds;
    private:
      ClientManager*  _manager;
      unsigned        _niovload;
      unsigned        _niovread;
      iovec*          _iovload;

    public slots:
      void set_chrome_visible(bool);
      void hide_stack();
    protected:
      void paintEvent(QPaintEvent*);

      QtPStack* _stack;
    private:
      QLayout*     _layout;
      QVBoxLayout* _layout3;
      //      QHBoxLayout* _layout4;
      bool         _chrome_changed;

    private:
      Semaphore*  _sem;

      bool _throttled;
      unsigned _denials;
      unsigned _attempts;

      bool     _reset;
    public:
      static void use_scroll_area(bool);
    };
  };
};

#endif      
