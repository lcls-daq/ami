#ifndef AmiQt_ChannelMath_hh
#define AmiQt_ChannelMath_hh

#include <QtGui/QWidget>

#include <QtCore/QStringList>

class QLineEdit;

namespace Ami {
  class AbsFilter;
  class AbsOperator;
  namespace Qt {
    class ChannelDefinition;
    class ChannelMath : public QWidget {
      Q_OBJECT
    public:
      ChannelMath(const QStringList&);
      ~ChannelMath();
    public:
      const QString&   expr() const;
      bool             resolve(ChannelDefinition* ch[],
			       int*  signatures,
			       int   nch,
			       const Ami::AbsFilter&);
      const Ami::AbsFilter& filter() const;
      const Ami::AbsOperator& op  () const;
    public slots:
      void calc();
    private:
      QLineEdit*    _expr;
      bool          _changed;
      QStringList   _names;
      QList<int>    _signatures;
      Ami::AbsFilter*   _filter;
      Ami::AbsOperator* _operator;
      QStringList   _uses;
    };
  };
};

#endif
