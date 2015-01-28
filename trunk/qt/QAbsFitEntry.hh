#ifndef AmiQt_QAbsFitEntry_hh
#define AmiQt_QAbsFitEntry_hh

class QwtPlot;

namespace Ami {
  class Entry;
  namespace Qt {
    class QAbsFitEntry {
    public:
      virtual ~QAbsFitEntry() {}
    public:
      virtual void attach(QwtPlot*)=0;
      virtual void fit   (const Entry&)=0;
    };
  };
};

#endif
