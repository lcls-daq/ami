#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

#include "ami/qt/XtcFileClient.hh"
#include "ami/qt/DetectorSelect.hh"
#include "ami/qt/Path.hh"
#include "ami/app/AnalysisFactory.hh"
#include "ami/app/XtcClient.hh"
#include "ami/server/ServerManager.hh"
#include "ami/service/Ins.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/UserAnalysis.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <QtGui/QApplication>

using namespace Ami;

typedef Pds::DetInfo DI;

static void usage(char* progname) {
  fprintf(stderr,
	  "Usage: %s -e <experiment name> -p <xtc path>\n"
	  "         [-i <interface address>]\n"
	  "         [-s <server mcast group>]\n"
	  "         [-L <user plug-in path>]\n", progname);
}


int main(int argc, char* argv[]) {
  int c;
  unsigned interface   = 0x7f000001;
  unsigned serverGroup = 0xefff2000;
  bool offline=false;
  const char* path = "/reg/d/pcds/amo/offline";
  //  plug-in module
  UserAnalysis* user = 0;
  create_t*  create_user  = 0;
  destroy_t* destroy_user = 0;

  while ((c = getopt(argc, argv, "?hs:L:f:p:")) != -1) {
    switch (c) {
    case 's':
      { in_addr inp;
	if (inet_aton(optarg, &inp))
	  serverGroup = ntohl(inp.s_addr);
	break; }
    case 'L':
      {
	void* handle = dlopen(optarg, RTLD_LAZY);
	if (!handle) break;

	// reset errors
	const char* dlsym_error;
	dlerror();

	// load the symbols
	create_t* c_user = (create_t*) dlsym(handle, "create");
	if ((dlsym_error = dlerror())) {
	  fprintf(stderr,"Cannot load symbol create: %s\n",dlsym_error);
	  break;
	}

	dlerror();
	destroy_t* d_user = (destroy_t*) dlsym(handle, "destroy");
	if ((dlsym_error = dlerror())) {
	  fprintf(stderr,"Cannot load symbol destroy: %s\n",dlsym_error);
	  break;
	}

	create_user  = c_user;
	destroy_user = d_user;
      }
      break;
    case 'f':
      Ami::Qt::Path::setBase(optarg);
      break;
    case 'p':
      path = optarg;
      break;
    case '?':
    case 'h':
    default:
      usage(argv[0]);
      exit(0);
    }
  }

  if (!interface) {
    usage(argv[0]);
    exit(0);
  }

  QApplication app(argc, argv);

  ServerManager   srv(interface, serverGroup);

  if (create_user)
    user = create_user();

  FeatureCache    features;
  AnalysisFactory factory(features, srv, user);

  XtcClient     myClient(features, factory, user, offline);
  Ami::Qt::XtcFileClient input(myClient, path);

  srv.serve(factory);
  srv.start();  // run in another thread
  //  srv.routine();  // run in this thread

  input.show();

  Ami::Qt::DetectorSelect output("AMO Offline Monitoring",interface,interface,serverGroup);
  output.show();

  app.exec();

  srv.stop();   // terminate the other thread
  srv.dont_serve();

  if (user)
    destroy_user(user);

  return 1;
}
