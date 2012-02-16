/*
 * Mutex for implementing monitors (java-style synchronization).
 * Thread that locks must be same thread that unlocks.
 * Not recursive (thread cannot a mutex it already has locked).
 */

#define DEBUG 0

#include "Mutex.hh"
#include <iostream>

using namespace Ami;
using namespace std;

static void fail() {
  *((char *)0) = 0;
}

Mutex::Mutex(const char *name) :
  _name(name),
  _owner(0) {
  pthread_cond_init(&_condition, NULL);
  pthread_mutex_init(&_mutex, NULL);
}


Mutex::~Mutex() {
  pthread_mutex_lock(&_mutex);
  if (_owner) {
    cout << "Mutex::~Mutex: " << _name << " still locked by " << pthread_self() << endl;
    fail();
  }
}

void Mutex::lock() {
  pthread_mutex_lock(&_mutex);
  while (_owner) {
    if (_owner == pthread_self()) {
      cout << "Mutex::lock: " << _name << " already locked by " << pthread_self() << endl;
      fail();
    }
#if DEBUG
    cout << "Mutex::lock: " << _name << ": waiting" << endl;
#endif
    pthread_cond_wait(&_condition, &_mutex);
#if DEBUG
    cout << "Mutex::lock: " << _name << ": trying again" << endl;
#endif
  }
  _owner = pthread_self();
#if DEBUG
  cout << "Mutex::lock: " << _name << " now locked by " << pthread_self() << endl;
#endif
  pthread_mutex_unlock(&_mutex);
}

void Mutex::unlock() {
  pthread_mutex_lock(&_mutex);
  if (_owner != pthread_self()) {
    cout << "Mutex::unlock: " << _name << " is not locked by " << pthread_self() << "but rather by " << _owner << endl;
    fail();
  }
  _owner = 0;
#if DEBUG
  cout << "Mutex::unlock: " << _name << " unlocked by " << pthread_self() << endl;
#endif
  pthread_cond_broadcast(&_condition);
  pthread_mutex_unlock(&_mutex);
}
