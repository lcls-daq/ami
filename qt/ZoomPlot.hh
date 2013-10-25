#ifndef AmiQt_ZoomPlot_hh
#define AmiQt_ZoomPlot_hh

//=========================================================
//
//  ZoomPlot for the Analysis and Monitoring Implementation
//
//  Filter configures the server-side filtering
//  Math   configures the server-side operations
//
//=========================================================

#include "ami/qt/QtPWidget.hh"
#include "ami/data/ConfigureRequestor.hh"

#include <QtCore/QString>

class QPrinter;

namespace Ami {
  class Cds;
  class AbsOperator;
  namespace Qt {
    class ChannelDefinition;
    class ImageDisplay;
    class ZoomPlot : public QtPWidget {
      Q_OBJECT
    public:
      ZoomPlot(QWidget*,
	       const QString&,
	       unsigned          input_channel,
	       Ami::AbsOperator* op,
               bool              scale_xy=false);
      ZoomPlot(QWidget*,
	       const QString&,
               bool              scale_xy=false);
      ZoomPlot(QWidget*,const char*& p);
      ~ZoomPlot();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      void configure(char*& p, 
		     unsigned input, 
		     unsigned& output);
      void configure(char*& p, 
		     unsigned input, 
		     unsigned& output,
		     ChannelDefinition* ch[], 
		     int* signatures, 
		     unsigned nchannels);
      void setup_payload(Cds&);
      void update();
    private:
      QString          _name;
      unsigned         _input;
      bool             _scalexy;
      unsigned         _signature;
      ImageDisplay*    _frame;
      AbsOperator*     _op;
      ConfigureRequestor _req;
    };
  };
};

#endif      
