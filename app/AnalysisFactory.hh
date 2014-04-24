#ifndef Ami_AnalysisFactory_hh
#define Ami_AnalysisFactory_hh

#include "ami/server/Factory.hh"
#include "ami/data/UserModuleDriver.hh"
#include "ami/data/Analysis.hh"
#include "ami/data/Cds.hh"
#include "ami/service/DumpSource.hh"

#include <list>
#include <vector>

namespace Ami {

  class FeatureCache;
  class Message;
  class Server;
  class ServerManager;
  class UserModule;
  class EventFilter;

  class AnalysisFactory : public Factory,
			  public UserModuleDriver,
			  public DumpSource {
  public:
    AnalysisFactory(std::vector<FeatureCache*>&,
		    ServerManager&,
		    std::list<UserModule*>&,
                    EventFilter&);
    ~AnalysisFactory();
  public:
    std::vector<FeatureCache*>& features();
    Cds& discovery();
    Cds& hidden   ();
    void discover (bool waitForConfigure);
    void discover_wait();
    void configure(unsigned, const Message&, const char*, Cds&, bool post_svc);
    void analyze  ();
    void remove   (unsigned);
  public:
    void recreate (UserModule*);
  public:
    std::string dump() const;
  private:
    ServerManager& _srv;
    Cds       _cds;
    Cds       _ocds;
    AnList    _analyses;
    AnList    _post_analyses;
    std::vector<FeatureCache*>& _features;
    typedef std::list<UserModule*> UList;
    UList&        _user;
    typedef std::vector<Cds*> CList;
    CList         _user_cds;
    EventFilter&  _filter;
    pthread_mutex_t _mutex;
    pthread_cond_t _condition;
    bool _waitingForConfigure;
  };

};

#endif
