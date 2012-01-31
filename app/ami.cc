#include "ami/app/AmiApp.hh"

using Ami::AmiApp;

static void usage(char* progname) {
  fprintf(stderr,
	  "Usage: %s -p <partitionTag>\n"
	  "          -n <partitionIndex>\n"
	  "          -i <interface>\n"
	  "          -s <server mcast group>\n"
	  "          -L <user module plug-in path>\n"
	  "          [-f] (offline) [-h] (help)\n", progname);
}

int main(int argc, char* argv[]) {
  int c;
  unsigned interface   = 0x7f000001;
  unsigned serverGroup = 0;
  char* partitionTag = 0;
  int   partitionIndex = 0;
  bool offline=false;
  std::vector<char *> module_names;

  while ((c = getopt(argc, argv, "?hfp:n:i:s:L:")) != -1) {
    switch (c) {
      case 'f':
        offline=true;
        break;
      case 'i':
        interface = AmiApp::parse_interface(optarg);
        break; 
      case 's':
        serverGroup = AmiApp::parse_ip(optarg);
        break;
      case 'p':
        partitionTag = optarg;
        break;
      case 'n':
        partitionIndex = strtoul(optarg,NULL,0);
        break;
      case 'L':
        module_names.push_back(optarg);
        break;
      case '?':
      case 'h':
      default:
        usage(argv[0]);
        exit(0);
    }
  }

  if (!partitionTag || !interface || !serverGroup) {
    usage(argv[0]);
    exit(0);
  }

  return AmiApp::run(partitionTag, serverGroup, module_names, interface, partitionIndex, offline);
}
