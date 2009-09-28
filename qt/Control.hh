#ifndef AmiQt_Control_hh
#define AmiQt_Control_hh


#include <QtGui/QWidget>
#include "ami/service/Timer.hh"

class QLabel;

namespace Ami {
  namespace Qt {
    class Requestor;
    class Control : public QWidget,
		    public Timer {
      Q_OBJECT
    public:
      Control(Requestor&);
      ~Control();
    public:  // Timer interface
      void     expired();
      Task*    task   ();
      unsigned duration  () const;
      unsigned repetitive() const;
    public slots:
      void run   (bool);
      void single();
    private:
      Requestor&  _client;
      Task*    _task;
      unsigned _repeat;
      QLabel*  _status;
    };
  };
};

#endif
