#ifndef Ami_Cdu_hh
#define Ami_Cdu_hh

namespace Ami {
  class Cds;

  class Cdu {
  public:
    Cdu();
    virtual ~Cdu();
  public:
    virtual void clear_payload() = 0;
  public:
    void subscribe(Cds& cds);
  private:
    Cds* _cds;
  };
};

#endif
