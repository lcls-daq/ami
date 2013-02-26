#ifndef AmiQt_EnvOverlay_hh
#define AmiQt_EnvOverlay_hh

#include "ami/qt/QtOverlay.hh"

#include "ami/data/ConfigureRequest.hh"
#include "ami/data/ConfigureRequestor.hh"

#include <QtCore/QString>

namespace Ami {
  class AbsFilter;
  class Cds;
  class DescEntry;
  class EntryScalarRange;
  namespace Qt {
    class QtBase;
    class QtPlot;
    class SharedData;
    class EnvOverlay : public QtOverlay {
    public:
      EnvOverlay(OverlayParent&   parent,
                 QtPlot&          frame,
                 const AbsFilter& filter,
                 DescEntry*       desc,
                 Ami::ScalarSet   set,
                 SharedData*      shared=0);
      EnvOverlay(OverlayParent&   parent,
                 const char*&    p);
      ~EnvOverlay();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void dump(FILE*) const;
      const QtBase* base() const;
    public:
      void configure(char*& p, unsigned input, unsigned& output);
      void setup_payload(Cds&);
      void update();
    private:
      void _attach();
    private:
      QtPlot*            _frame;
      QString            _frame_name;
      AbsFilter*         _filter;
      DescEntry*         _desc;
      Ami::ScalarSet     _set;
      unsigned           _output_signature;
      QtBase*            _plot;
      ConfigureRequestor _req;
      EntryScalarRange*  _auto_range;
      int                _order;
      SharedData*        _shared;
    };
  };
};

#endif
		 
