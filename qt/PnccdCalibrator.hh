#ifndef AmiQt_PnccdCalibrator_hh
#define AmiQt_PnccdCalibrator_hh

#include "ami/qt/QtPWidget.hh"
#include "ami/data/ConfigureRequestor.hh"

class QPushButton;
class QLineEdit;

namespace Ami {

  class AbsOperator;
  class Cds;
  class Entry;
  class EntryImage;

  namespace Qt {
    class ChannelDefinition;
    class PnccdClient;
    class ZoomPlot;

    class PnccdCalibrator : public QtPWidget {
      Q_OBJECT
    public:
      PnccdCalibrator(PnccdClient* parent);
      ~PnccdCalibrator();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void snapshot(const QString&) const;
      void save_plots(const QString&) const;
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels);
      void setup_payload(Cds&);
      void update();
    public slots:
      void acquire ();
      void writecal();   // configure the plot
    signals:
      void changed();
    private:
      PnccdClient*  _parent;

      class Param {
      public:
        Param(QWidget*, QString, AbsOperator*);
        ~Param();
      public:
        void configure    (char*& p, unsigned, unsigned&);
        void setup_payload(Ami::Cds& cds);
        void update       ();
        void acquire      (bool);
        void hide         ();
        const EntryImage* entry();
      private:
        ZoomPlot*          _plot;
        AbsOperator*       _op;
        ConfigureRequestor _req;
        unsigned           _signature;
        const EntryImage*  _entry;
        EntryImage*        _result;
      };

      Param         _ped;
      Param         _noise;

      bool          _acquiring;
      bool          _fnBox_state;
      bool          _npBox_state;

      QPushButton*  _acqB;
      QPushButton*  _saveB;
      QLineEdit*    _factor;
    };
  };
};

#endif
