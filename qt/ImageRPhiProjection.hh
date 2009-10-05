#ifndef AmiQt_ImageRPhiProjection_hh
#define AmiQt_ImageRPhiProjection_hh

#include "ami/qt/QtPWidget.hh"

#include <QtCore/QString>
#include <QtCore/QStringList>

class QLineEdit;
class QButtonGroup;

#include <list>

namespace Ami {

  class AbsOperator;
  class Cds;
  class Entry;

  namespace Qt {
    class AnnulusCursors;
    class ChannelDefinition;
    class ImageFrame;
    class ProjectionPlot;

    class ImageRPhiProjection : public QtPWidget {
      Q_OBJECT
    public:
      ImageRPhiProjection(QWidget* parent,
			  ChannelDefinition* channels[], unsigned nchannels, ImageFrame&);
      ~ImageRPhiProjection();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels);
      void setup_payload(Cds&);
      void update();
    public slots:
      void set_channel(int); // set the source
      void plot        ();   // configure the plot
      void configure_plot();
      void remove_plot (QObject*);
      virtual void setVisible(bool);
    signals:
      void changed();
    private:
      void _set_cursor  (double, double);
    private:
      ChannelDefinition** _channels;
      unsigned _nchannels;
      unsigned _channel;

      ImageFrame&  _frame;

      QLineEdit* _title;
      AnnulusCursors* _annulus;
      QButtonGroup* _axis;
      QButtonGroup* _norm;

      Ami::AbsOperator* _operator;

      std::list<ProjectionPlot*> _pplots;
    };
  };
};

#endif
