#ifndef AmiQt_EdgeCursor_hh
#define AmiQt_EdgeCursor_hh

#include "ami/qt/Cursors.hh"

#include <QtGui/QWidget>

class QString;
class QLineEdit;
class QPushButton;
class QwtPlotMarker;

namespace Ami {
  namespace Qt {
    class PlotFrame;
    class EdgeCursor : public QWidget,
		       public Cursors {
      Q_OBJECT
    public:
      EdgeCursor(const QString&, PlotFrame&);
      ~EdgeCursor();
    public:
      double value() const;
    private:
      void _set_cursor(double,double);
    public slots:
      void set_value();    
      void grab();
      void show_in_plot(bool);
    signals:
      void changed();
    private:
      QString    _name;
      QLineEdit* _input;
      QPushButton* _showB;
      QwtPlotMarker* _marker;
    };
  };
};

#endif
