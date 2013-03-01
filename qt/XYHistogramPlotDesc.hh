#ifndef AmiQt_XYHistogramPlotDesc_hh
#define AmiQt_XYHistogramPlotDesc_hh

#include <QtGui/QWidget>

#include "ami/data/ConfigureRequestor.hh"

class QButtonGroup;
class QLabel;

namespace Ami {
  class XYHistogram;
  class Cds;
  class EntryScalarRange;
  namespace Qt {
    class DescTH1F;
    class RectangleCursors;
    class ChannelDefinition;

    class XYHistogramPlotDesc : public QWidget {
    public:
      XYHistogramPlotDesc(QWidget* parent);
      ~XYHistogramPlotDesc();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      Ami::XYHistogram* desc(const char*) const;
    private:
      DescTH1F* _desc;

      //  Interface for displaying pixel range
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
      QLabel*            _pixel_range;
      unsigned           _output;
      ConfigureRequestor _req;
      EntryScalarRange*  _entry;
    };
  };
};

#endif
