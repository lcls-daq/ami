#ifndef Pds_DESC_HH
#define Pds_DESC_HH

namespace Ami {

  class Desc {
  public:
    Desc(const char* name);
    Desc(const Desc& desc);
    ~Desc();

    const char* name() const;
    int signature() const;
    unsigned nentries() const;
    void nentries(unsigned);
    void added();
    void reset();
    void signature(int i);

  private:
    enum {NameSize=128};
    char     _name[NameSize];
    int      _signature;
    unsigned _nentries;
  };
};

#endif
