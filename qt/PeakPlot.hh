#ifndef AmiQt_PeakPlot_hh
#define AmiQt_PeakPlot_hh

//=========================================================
//
//  PeakPlot for the Analysis and Monitoring Implementation
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
    class PeakPlot : public QWidget {
      Q_OBJECT
    public:
      PeakPlot(const QString&,
	       unsigned input_channel,
	       unsigned threshold);
      ~PeakPlot();
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
      unsigned         _threshold;
      unsigned         _signature;
      ImageDisplay*    _frame;
    };
  };
};

#endif      
