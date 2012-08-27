#ifndef AmiQt_EdgeCursor_hh
#define AmiQt_EdgeCursor_hh

#include "ami/qt/Cursors.hh"

#include "ami/qt/QtPWidget.hh"

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
      EdgeCursor(const QString&, PlotFrame&, QtPWidget* =0);
      ~EdgeCursor();
    public:
      void load(const char*&);
      void save(char*&) const;
    public:
      double value() const;
      void   value(double);
      void mousePressEvent  (double,double);
      void mouseMoveEvent   (double,double);
      void mouseReleaseEvent(double,double);
      void setName(QString &name);
    public slots:
      void set_value();    
      void grab();
      void show_in_plot(bool);
    signals:
      void changed();
    private:
      PlotFrame& _frame;
      QtPWidget* _frameParent;
      QString    _name;
      QLineEdit* _input;
      QPushButton* _showB;
      QwtPlotMarker* _marker;
    };
  };
};

#endif
