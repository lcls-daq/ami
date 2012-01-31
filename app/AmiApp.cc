#include <sys/ioctl.h>
#include <net/if.h>
#include <dlfcn.h>

#include "ami/app/AmiApp.hh"
#include "ami/app/XtcClient.hh"
#include "ami/app/XtcShmClient.hh"
#include "ami/app/AnalysisFactory.hh"
#include "ami/app/EventFilter.hh"

#include "ami/server/ServerManager.hh"
#include "ami/service/Ins.hh"

#include "pdsdata/xtc/DetInfo.hh"

using namespace Ami;

typedef Pds::DetInfo DI;

template <class U, class C>
void AmiApp::load_syms(std::list<U*>& user, char* arg)
{
  for(const char* p = strtok(arg,","); p!=NULL; p=strtok(NULL,",")) {
    
    printf("dlopen %s\n",p);

    void* handle = dlopen(p, RTLD_LAZY);
    if (!handle) {
      printf("dlopen failed : %s\n",dlerror());
      break;
    }

    // reset errors
    const char* dlsym_error;
    dlerror();

    // load the symbols
    C* c_user = (C*) dlsym(handle, "create");
    if ((dlsym_error = dlerror())) {
      fprintf(stderr,"Cannot load symbol create: %s\n",dlsym_error);
      break;
    }
          
//     dlerror();
//     destroy_t* d_user = (destroy_t*) dlsym(handle, "destroy");
//     if ((dlsym_error = dlerror())) {
//       fprintf(stderr,"Cannot load symbol destroy: %s\n",dlsym_error);
//       break;
//     }

    user.push_back( c_user() );
  }
}

unsigned AmiApp::parse_ip(const char* ipString) {
  unsigned ip = 0;
  in_addr inp;
  if (inet_aton(ipString, &inp)) {
    ip = ntohl(inp.s_addr);
  }
  return ip;
}

unsigned AmiApp::parse_interface(const char* interfaceString) {
  unsigned interface = parse_ip(interfaceString);
  if (interface == 0) {
    int so = socket(AF_INET, SOCK_DGRAM, 0);
    if (so < 0) {
      perror("Failed to open socket\n");
      return 0;
    }
    ifreq ifr;
    strcpy(ifr.ifr_name, interfaceString);
    int rv = ioctl(so, SIOCGIFADDR, (char*)&ifr);
    close(so);
    if (rv != 0) {
      printf("Cannot get IP address for network interface %s.\n",interfaceString);
      return 0;
    }
    interface = ntohl( *(unsigned*)&(ifr.ifr_addr.sa_data[2]) );
  }
  printf("Using interface %s (%d.%d.%d.%d)\n",
         interfaceString,
         (interface>>24)&0xff,
         (interface>>16)&0xff,
         (interface>> 8)&0xff,
         (interface>> 0)&0xff);
  return interface;
}

int AmiApp::run(char *partitionTag, unsigned serverGroup, std::vector<char *> module_names, unsigned interface, int partitionIndex, bool offline) {
  //  plug-in modules
  std::list<UserModule*> user_mod;
  for (unsigned i = 0; i < module_names.size(); i++) {
    load_syms<UserModule,create_m>(user_mod, module_names[i]);
  }

  ServerManager   srv(interface, serverGroup);

  std::vector<FeatureCache*> features;
  for(unsigned i=0; i<Ami::NumberOfSets; i++)
    features.push_back(new FeatureCache);
  EventFilter     filter(user_mod,*features[PostAnalysis]);
  AnalysisFactory factory(features, srv, user_mod, filter);

  XtcClient myClient(features, factory, user_mod, filter, offline);
  XtcShmClient input(myClient, partitionTag, partitionIndex);

  srv.manage(input);
  srv.serve(factory);
  //  srv.start();  // run in another thread
  srv.routine();  // run in this thread
  //  srv.stop();   // terminate the other thread
  srv.dont_serve();

  for(std::list<UserModule*>::iterator it=user_mod.begin(); 
      it!=user_mod.end(); it++)
    delete (*it);

  return 1;
}
