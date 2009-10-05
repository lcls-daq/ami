#ifndef AmiQt_QtPersistent_hh
#define AmiQt_QtPersistent_hh

#include <QtCore/QString>

namespace Ami {
  namespace Qt {
    class QtPersistent {
    public:
      virtual ~QtPersistent() {}
    public:
      virtual void save(char*& p) const = 0;
      virtual void load(const char*& p) = 0;
    public:
      static void insert(char*&, const QString&);
      static void insert(char*&, unsigned);
      static void insert(char*&, int);
      static void insert(char*&, double);
      static void insert(char*&, bool);
      static int     extract_i(const char*&);
      static double  extract_d(const char*&);
      static QString extract_s(const char*&);
      static bool    extract_b(const char*&);
    };
  };
};

#endif
