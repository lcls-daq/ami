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

#include "ami/qt/QtPWidget.hh"
#include "ami/data/ConfigureRequestor.hh"

#include <QtCore/QString>

namespace Ami {
  class AbsOperator;
  class Cds;
  namespace Qt {
    class ChannelDefinition;
    class ImageDisplay;
    class ImageXYProjection;
    class ImageRPhiProjection;
    class ImageContourProjection;

    class PeakPlot : public QtPWidget {
      Q_OBJECT
    public:
      PeakPlot(QWidget* parent,
	       const QString&,
	       unsigned input_channel,
               AbsOperator* op);
      PeakPlot(QWidget*, const char*&);
      ~PeakPlot();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void save_plots(const QString&) const;
    public:
      void configure(char*& p, 
                     unsigned input, 
                     unsigned& output,
                     ChannelDefinition* ch[], 
                     int* signatures, 
                     unsigned nchannels);
      void setup_payload(Cds&);
      void update();
    public slots:
      void update_configuration();
    signals:
      void description_changed();
    private:
      void _layout();
    private:
      QString          _name;
      unsigned         _input;
      AbsOperator*     _op;
      unsigned         _signature;

      enum {NCHANNELS=4};
      ChannelDefinition* _channels[NCHANNELS];
      ImageDisplay*    _frame;
      ImageXYProjection*      _xyproj;
      ImageRPhiProjection*    _rfproj;
      ImageContourProjection* _cntproj;
      unsigned           _showMask;
      ConfigureRequestor _req;

    public slots:
      void set_chrome_visible(bool);
    protected:
      void paintEvent(QPaintEvent*);
    private:
      QLayout* _chrome_layout;
      bool     _chrome_changed;
    };
  };
};

#endif      
