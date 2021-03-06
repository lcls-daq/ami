#include "ami/app/AmiApp.hh"
#include "ami/app/CmdLineTools.hh"
#include "ami/server/CollectionServerManager.hh"
#include "ami/service/Ins.hh"
#include "ami/service/DumpCollector.hh"

#include <stdlib.h>
#include <getopt.h>

using Ami::AmiApp;

static void usage(char* progname) {
  fprintf(stderr,
	  "Usage: %s -I <client interface>\n"	
	  "          -S <client mcast group>\n"
          "          -i <server interface>\n"
	  "          -s <server mcast group>\n", progname);
}

int main(int argc, char* argv[]) {
  int c;
  unsigned client_interface   = 0x7f000001;
  unsigned server_interface   = 0x7f000001;
  unsigned client_group = 0;
  unsigned server_group = 0;
  bool     parseValid = true;

  while ((c = getopt(argc, argv, "?hI:i:S:s:")) != -1) {
    switch (c) {
      case 'I':
        client_interface = Ami::Ins::parse_interface(optarg);
        break; 
      case 'i':
        server_interface = Ami::Ins::parse_interface(optarg);
        break; 
      case 'S':
        client_group = Ami::Ins::parse_ip(optarg);
        break;
      case 's':
        server_group = Ami::Ins::parse_ip(optarg);
        break;
      case '?':
      case 'h':
      default:
        usage(argv[0]);
        exit(0);
    }
  }

  parseValid &= (optind==argc);

  if (!parseValid || !server_group || !client_group) {
    usage(argv[0]);
    exit(0);
  }

  Ami::CollectionServerManager srv(client_interface, 
                                   client_group,
                                   server_interface, 
                                   server_group);

  Ami::DumpCollector dump(client_interface,
                          server_interface);
  dump.add(srv);

  srv.serve();
  srv.routine();
  srv.dont_serve();
  return 1;
}
