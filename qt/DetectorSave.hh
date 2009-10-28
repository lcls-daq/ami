#ifndef AmiQt_DetectorSave_hh
#define AmiQt_DetectorSave_hh

#include "ami/qt/DetectorGroup.hh"

namespace Ami {
  namespace Qt {
    class DetectorSave : public DetectorGroup {
    public:
      DetectorSave(QWidget* parent,
		   QtTopWidget**,
		   const char**,
		   int);
      ~DetectorSave();
    private:
      void _init ();
      void _apply(QtTopWidget&, const QString&);
    private:
      QString _prefix;
    };
  };
};

#endif
