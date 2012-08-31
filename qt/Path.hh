#ifndef AmiQt_Path_hh
#define AmiQt_Path_hh

#include <QtCore/QString>

#include <stdlib.h>

class QWidget;

namespace Ami {
  namespace Qt {
    class Path {
    public:
      static void    setBase(const QString&);
      static const QString& base();
      static FILE*   helpFile();
      static FILE*   saveDataFile(QWidget*);
      static FILE*   saveReferenceFile(QWidget*,const QString&);      
      static QString loadReferenceFile(QWidget*,const QString&);
    };
  };
};

#endif
