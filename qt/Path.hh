#ifndef AmiQt_Path_hh
#define AmiQt_Path_hh

#include <QtCore/QString>

#include <stdlib.h>

namespace Ami {
  namespace Qt {
    class Path {
    public:
      static FILE*   saveDataFile();
      static FILE*   saveReferenceFile(const QString&);      
      static QString loadReferenceFile(const QString&);
    };
  };
};

#endif
