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

#include <QtGui/QWidget>

#include <QtCore/QString>

namespace Ami {
  class Cds;
  namespace Qt {
    class ChannelDefinition;
    class ImageDisplay;
    class ZoomPlot : public QWidget {
      Q_OBJECT
    public:
      ZoomPlot(QWidget*,
	       const QString&,
	       unsigned input_channel,
	       unsigned x0, 
	       unsigned y0,
	       unsigned x1,
	       unsigned y1);
      ~ZoomPlot();
    public:
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
      unsigned         _signature;
      unsigned         _x0, _y0, _x1, _y1;
      ImageDisplay*    _frame;
    };
  };
};

#endif      
