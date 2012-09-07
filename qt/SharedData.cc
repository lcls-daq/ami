#include "ami/qt/SharedData.hh"

#include <QtCore/qatomic.h>

using namespace Ami::Qt;

SharedData::SharedData() : _ref(0) { printf("Created SD %p\n",this); }

SharedData::~SharedData() {}

void SharedData::signup() 
{
  q_atomic_increment(&_ref); 
  printf("SD inc %d\n",_ref); 
}

void SharedData::resign() 
{ 
  printf("SD dec %d\n",_ref); 
  if (!q_atomic_decrement(&_ref)) 
    delete this; 
}

