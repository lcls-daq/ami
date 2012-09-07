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
      int _ref;
    };

    inline SharedData::SharedData() : _ref(0) {}
    inline SharedData::~SharedData() {}
    inline void SharedData::signup() { q_atomic_increment(&_ref); }
    inline void SharedData::resign() { if (!q_atomic_decrement(&_ref)) delete this; }
  };
};

#endif
