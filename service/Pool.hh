#ifndef Ami_Pool_hh
#define Ami_Pool_hh

namespace Ami {
  class Pool {
  public:
    Pool(int sz);
    ~Pool();
  public:
    char* base  () const { return _buffer; }
    int   extent() const { return _current-_buffer; }
  public:
    char* reset  () { return _current=_buffer; }
    char* extend (int);
    void  reserve(int);
  private:
    void  _resize(int);
  private:
    char* _buffer;
    char* _current;
    int   _size;
  };
};

#endif
