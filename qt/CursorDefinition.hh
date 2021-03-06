#ifndef AmiQt_CursorDefinition_hh
#define AmiQt_CursorDefinition_hh

#include <QtGui/QWidget>

#include <QtCore/QString>

class QLineEdit;
class QwtPlot;
class QwtPlotMarker;

namespace Ami {
  namespace Qt {
    class Cursors;
    class CursorDefinition : public QWidget {
      Q_OBJECT
    public:
      CursorDefinition(const QString& name,
		       double    location,
		       Cursors& parent,
		       QwtPlot*  plot);
      CursorDefinition(const char*&,
		       Cursors& parent,
		       QwtPlot*  plot);
      ~CursorDefinition();
    public:
      QString name() const;
      double  location() const { return _location; }
    public:
      void save(char*&) const;
      void load(const char*&);
    signals:
      void changed();
    public slots:
      void show_in_plot(bool);
      void remove();
      void update();
    private:
      void _layout();
    private:
      QLineEdit* _name;
      double     _location;
      QLineEdit* _value;
      Cursors&   _parent;
      QwtPlot*       _plot;
      QwtPlotMarker* _marker;
    };
  };
};

#endif
