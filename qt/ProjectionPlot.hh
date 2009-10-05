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

#include <QtCore/QString>

namespace Ami {
  class Cds;
  class DescEntry;
  class AbsOperator;
  namespace Qt {
    class ChannelDefinition;
    class CursorsX;
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
      void save(char*& p) const;
      void load(const char*& p);
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
      QString           _name;
      unsigned          _input;
      Ami::AbsOperator* _proj;

      enum {NCHANNELS=4};
      ChannelDefinition* _channels[NCHANNELS];
      WaveformDisplay*   _frame;
      const DescEntry*   _input_entry;
      CursorsX*          _cursors;
    };
  };
};

#endif      
