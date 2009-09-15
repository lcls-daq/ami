#ifndef AmiQt_ChannelDefinition_hh
#define AmiQt_ChannelDefinition_hh

#include <QtGui/QWidget>

#include <QtGui/QColor>
#include <QtCore/QString>

class QButtonGroup;
class QLineEdit;

namespace Ami {
  class AbsOperator;
  class AbsTransform;
  class Cds;
  class Entry;
  namespace Qt {
    class Display;
    class Filter;
    class ChannelMath;
    class Transform;
    class ChannelDefinition : public QWidget {
      Q_OBJECT
    public:
      ChannelDefinition(const QString& name, Display& frame, const QColor&, bool init=false);
      ~ChannelDefinition();
    public:
      const QString& name() const { return _name; }
      const Filter&       filter   () const { return *_filter; }
      AbsTransform& transform();
      int           configure(char*& p, unsigned input, unsigned& output,
			      ChannelDefinition* ch[], int*, int);
      void          setup_payload  (Cds&);
    public slots:
      void load_reference();
      void show_plot(bool);
      void apply();
    signals:
      void reference_loaded(bool);
      void changed();
    private:
      QString       _name;
      Display&      _frame;
      QColor        _color;
      QButtonGroup* _plot_grp;
      Filter*       _filter;
      AbsOperator*  _operator;
      Transform*    _transform;
      ChannelMath*  _math;
      QLineEdit*    _interval;
      unsigned      _mode;
      unsigned      _output_signature;
      bool          _changed;
      QString       _ref_file;
    };
  };
};

#endif
      
