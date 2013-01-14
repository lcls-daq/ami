#ifndef AmiQt_ChannelDefinition_hh
#define AmiQt_ChannelDefinition_hh

#include "ami/qt/QtPWidget.hh"

#include <QtGui/QColor>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "ami/data/ConfigureRequest.hh"
#include "ami/data/ConfigureRequestor.hh"

class QButtonGroup;
class QLabel;
class QLineEdit;
class QComboBox;
class QCheckBox;

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
    class QtBase;
    class MaskDisplay;
    class ChannelDefinition : public QtPWidget {
      Q_OBJECT
    public:
      ChannelDefinition(QWidget* parent,
			const QString& name, 
			const QStringList& names,
			Display& frame, const QColor&, 
                        bool init,
                        QStringList ref = QStringList());
      ~ChannelDefinition();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      const QString&     name() const { return _name; }
      const Filter&      filter  () const { return *_filter; }
      const AbsOperator& oper    () const { return *_operator; }
      bool               is_shown() const { return _show; }
    public:
      AbsTransform& transform();
      int           configure(char*& p, unsigned input, unsigned& output,
			      ChannelDefinition* ch[], int*, int,
			      ConfigureRequest::Source = ConfigureRequest::Discovery);
    private:
      bool          _configure(char* p, unsigned input, unsigned output,
                               ChannelDefinition* ch[], int*, int,
                               ConfigureRequest::Source = ConfigureRequest::Discovery);
    public:
      void          setup_payload  (Cds&,bool=true);
      unsigned      output_signature() const;
    public slots:
      void load_reference();
      void show_plot(bool);
      void apply();
      void set_scale();
      void load_mask();
      void edit_mask();
      void update_interval();
    signals:
      void reference_loaded(bool);
      void changed();
      void newplot(bool);
      void show_plot_changed(bool);
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
      QLabel*       _intervalq;
      unsigned      _mode;
      unsigned      _output_signature;
      bool          _changed;
      QString       _ref_file;
      bool          _show;
      QtBase*       _plot;
      QComboBox*    _refBox;
      QLineEdit*    _scale;
      bool          _operator_is_ref;
      bool          _configured_ref;
      ConfigureRequestor _req;
      QCheckBox*    _maskB;
      QString       _mask_file;
      MaskDisplay*  _mask_display;
    };
  };
};

#endif
      
