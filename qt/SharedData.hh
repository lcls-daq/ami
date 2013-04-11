#ifndef AmiQt_SharedData_hh
#define AmiQt_SharedData_hh

#include <QtCore/qatomic.h>

namespace Ami {
  namespace Qt {
    class SharedData {
    public:
      SharedData();
      virtual ~SharedData();
    public:
      void signup();
      void resign();
    private:
#if QT_VERSION >= 0x040400
      QAtomicInt _ref;
#else
      volatile int _ref;
#endif
    };

    inline SharedData::SharedData() : _ref(0) {}
    inline SharedData::~SharedData() {}
#if QT_VERSION >= 0x040400
    inline void SharedData::signup() { _ref.ref(); }
    inline void SharedData::resign() { if (!_ref.deref()) delete this; }
#else
    inline void SharedData::signup() { q_atomic_increment(&_ref); }
    inline void SharedData::resign() { if (q_atomic_decrement(&_ref)==0) delete this; }
#endif
  };
};

#endif
