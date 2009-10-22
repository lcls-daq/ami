#ifndef AmiQt_EnvPlot_hh
#define AmiQt_EnvPlot_hh

#include "ami/qt/QtPlot.hh"
#include <QtCore/QString>

#include "ami/data/ConfigureRequest.hh"

#include <list>

namespace Ami {
  class Cds;
  class DescEntry;
  namespace Qt {
    class AxisArray;
    class ChannelDefinition;
    class EnvDefinition;
    class QtBase;
    class EnvPlot : public QtPlot {
      Q_OBJECT
    public:
      EnvPlot(QWidget*,
	      const QString& name,
	      DescEntry*     desc,
	      int            index0,
	      const QString& var);
      EnvPlot(QWidget*,const char*&);
      ~EnvPlot();
    public:
      void save(char*&) const;
      void load(const char*&);
    public:
      void configure(char*& p, unsigned input, unsigned& output);
      void setup_payload(Cds&);
      void update();
    private:
      void _dump(FILE*) const;
    private:
      DescEntry* _desc;
      int        _index0;
      QString    _var;
      
      unsigned _output_signature;

      QtBase*  _plot;
    };
  };
};

#endif
		 
