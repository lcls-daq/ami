#ifndef AmiQt_PlotStyle_hh
#define AmiQt_PlotStyle_hh

#include "ami/data/DescEntry.hh"

class QWidget;

namespace Ami {
  namespace Qt {

    class QtPlotStyle {
    public:
      QtPlotStyle();
      QtPlotStyle(const QtPlotStyle&);
      ~QtPlotStyle();
    public:
      int symbol_size () const { return _symbol_size; }
      int symbol_style() const { return _symbol_style; }
      int line_size   () const { return _line_size; }
      int line_style  () const { return _line_style; }
    public:
      void query(QWidget*);
      void save(char*&) const;
      void load(const char*&);
      void setPlotType(Ami::DescEntry::Type);
   private:
      int _symbol_size;
      int _symbol_style;
      int _line_size;
      int _line_style;
    };
  };
};

#endif
