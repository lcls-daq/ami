#ifndef Ami_AnalysisFactory_hh
#define Ami_AnalysisFactory_hh

#include "ami/server/Factory.hh"

#include "ami/data/Cds.hh"

#include <list>

namespace Ami {

  class Analysis;
  class FeatureCache;
  class Message;
  class Server;
  class ServerManager;
  class UserAnalysis;

  class AnalysisFactory : public Factory {
  public:
    AnalysisFactory(FeatureCache&,
		    ServerManager&,
		    UserAnalysis*);
    ~AnalysisFactory();
  public:
    FeatureCache& features();
    Cds& discovery();
    void discover ();
    void configure(unsigned, const Message&, const char*, Cds&);
    void analyze  ();
    void wait_for_configure();
  private:
    ServerManager& _srv;
    Cds       _cds;
    typedef std::list<Analysis*> AnList;
    AnList    _analyses;
    Semaphore _configured;
    Semaphore _sem;
    FeatureCache& _features;
    UserAnalysis* _user;
  };

};

#endif
