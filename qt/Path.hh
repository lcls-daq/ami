#ifndef AmiQt_Path_hh
#define AmiQt_Path_hh

#include <QtCore/QString>

#include <stdlib.h>

class QWidget;

namespace Ami {
  namespace Qt {
    class Path {
    public:
      static void    setBase   (const QString&);
      static void    setArchive(const QString&);
      static const QString& base   ();
      static const QString* archive();
      static FILE*   helpFile();
      static FILE*   saveDataFile(QWidget*);
      static FILE*   saveReferenceFile(QWidget*,const QString&);      
      static QString loadReferenceFile(QWidget*,const QString&);
      static void    saveAmiFile(QWidget*, const char*, int);
    };
  };
};

#endif
