#ifndef AmiQt_ProjectionPlot_hh
#define AmiQt_ProjectionPlot_hh

//=========================================================
//
//  ProjectionPlot for the Analysis and Monitoring Implementation
//
//  Filter configures the server-side filtering
//  Math   configures the server-side operations
//
//=========================================================

#include "ami/qt/QtPWidget.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/ConfigureRequestor.hh"

#include <QtCore/QString>

class QButtonGroup;

namespace Ami {
  class Cds;
  class DescEntry;
  class AbsOperator;
  class EntryScalarRange;
  namespace Qt {
    class ChannelDefinition;
    class CursorsX;
    class Fit;
    class PeakFit;
    class WaveformDisplay;

    class ProjectionPlot : public QtPWidget {
      Q_OBJECT
    public:
      ProjectionPlot(QWidget*,
		     const QString&,
		     unsigned input_channel,
		     Ami::AbsOperator*);
      ProjectionPlot(QWidget*,const char*&);
      ~ProjectionPlot();
    public:
      virtual void save(char*& p) const;
      virtual void load(const char*& p);
      void save_plots(const QString&) const;
    public:
      unsigned channel() const { return _input; }
    public:
      void configure(char*& p, 
		     unsigned input, 
		     unsigned& output,
                     ConfigureRequest::Source s=ConfigureRequest::Analysis);
      void configure(char*& p, 
		     unsigned input, 
		     unsigned& output,
		     ChannelDefinition* ch[], 
		     int* signatures, 
		     unsigned nchannels);
      void setup_payload(Cds&);
      void update();
      void dump(FILE*) const;
    public slots:
      void update_configuration();
    signals:
      void description_changed();
    private:
      void _layout();
    public:
      QString           _name;
    private:
      unsigned          _input;
      unsigned          _output;
      Ami::AbsOperator* _proj;

      enum {NCHANNELS=4};
      ChannelDefinition* _channels[NCHANNELS];
      QButtonGroup*      _showPlotBoxes;
      WaveformDisplay*   _frame;
      const DescEntry*   _input_entry;
      CursorsX*          _cursors;
      PeakFit*           _peakfit;
      Fit*               _fit;

      unsigned           _showMask;
      ConfigureRequestor _req;

      const EntryScalarRange*  _auto_range;

    public slots:
      void set_chrome_visible(bool);
    protected:
      void paintEvent(QPaintEvent*);
    private:
      QLayout*     _chrome_layout;
      bool         _chrome_changed;
    };
  };
};

#endif      
