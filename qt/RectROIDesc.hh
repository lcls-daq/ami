#ifndef AmiQt_RectROIDesc_hh
#define AmiQt_RectROIDesc_hh

#include "ami/qt/RectangleCursors.hh"
#include <QtGui/QWidget>
#include <vector>

class QComboBox;
class QPushButton;

namespace Ami {
  class Cds;
  namespace Qt {
    class ImageFrame;
    class Rect;
    class RectROI;
    class QtPWidget;
    class ChannelDefinition;

    class RectROIDesc : public QWidget {
      Q_OBJECT
    public:
      RectROIDesc(QtPWidget&,
		  ImageFrame&,
		  unsigned,
		  RectangleCursors::LayoutStyle=RectangleCursors::Standard);
      ~RectROIDesc();
    public:
      void save(char*&) const;
      void load(const char*&);
      void save_plots(const QString&) const;
      void snapshot(const QString&) const;
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* channels[], int* signatures, unsigned nchannels);
      void update();
      void setup_payload(Ami::Cds&);
      RectROI& roi(unsigned);
    public:
      unsigned iroi(unsigned) const;
      const std::vector<RectROI*>& rois() const;
    public slots:
      void update_range();
      void new_roi     ();
      void select_roi  (int);
    signals:
      void changed();
      void rchanged();
      void roi_changed(int);
    protected:
      void showEvent(QShowEvent*);
      void hideEvent(QHideEvent*);
    private:
      ImageFrame&       _frame;
      unsigned          _nchannels;
      RectangleCursors* _rectangle;
      std::vector<Rect*>    _rect;
      std::vector<RectROI*> _rois;
      QComboBox*            _roiBox;
      QPushButton*          _roiButton;
    };
  };
};

#endif
