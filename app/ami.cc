#include "ami/app/AmiApp.hh"
#include "ami/app/CmdLineTools.hh"
#include "ami/service/Ins.hh"
#include "ami/event/EpicsXtcReader.hh"
#include "ami/event/EventHandler.hh"
#include "ami/event/Calib.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using Ami::AmiApp;
using Ami::CmdLineTools;

static void usage(char* progname) {
  fprintf(stderr,
	  "Usage: %s -p <partitionTag>\n"
	  "          -n <partitionIndex>\n"
	  "          -i <interface>\n"
	  "          -s <server mcast group>\n"
	  "          -L <user module plug-in path>\n"
    "          [-D (post detector diagnostics)]\n"
    "          [-Q <pixels>] (set resolution)\n"
    "          [-R] (set full resolution)\n"
    "          [-O] (disable legacy online pedestal corrections)\n"
    "          [-E <expt name>] (set experiment for offline calib access)\n"
    "          [-c <calibdir>] (set the calibdir path for offline calib access)\n"
    "          [-a <include|only>] (include EPICS aliases [only])\n"
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
  bool parseValid=true;

  while ((c = getopt(argc, argv, "?hfDa:Q:ROE:c:p:n:i:s:L:")) != -1) {
    switch (c) {
    case 'f':
      offline=true;
      break;
    case 'i':
      interface = Ami::Ins::parse_interface(optarg);
      break; 
    case 's':
      serverGroup = Ami::Ins::parse_ip(optarg);
      break;
    case 'p':
      partitionTag = optarg;
      break;
    case 'n':
      parseValid &= CmdLineTools::parseInt(optarg,partitionIndex);
      break;
    case 'L':
      module_names.push_back(optarg);
      break;
    case 'R':
      Ami::EventHandler::enable_full_resolution(true);
      break;
    case 'O':
      Ami::Calib::use_online(false);
      break;
    case 'Q':
      { unsigned arg;
        parseValid &= CmdLineTools::parseUInt(optarg,arg);
        Ami::EventHandler::limit_resolution(arg); }
      break;
    case 'E':
      Ami::Calib::use_offline(true);
      Ami::Calib::set_experiment(optarg);
      break;
    case 'c':
      Ami::Calib::set_offline_root(optarg);
      break;
    case 'D':
      Ami::EventHandler::post_diagnostics(true);
      break;
    case 'a':
      Ami::EpicsXtcReader::use_alias(strcmp(optarg,"only")==0);
      break;
    case '?':
    case 'h':
    default:
      usage(argv[0]);
      exit(0);
    }
  }

  parseValid &= (optind==argc);

  if (!parseValid || !partitionTag || !interface || !serverGroup) {
    usage(argv[0]);
    exit(0);
  }

  return AmiApp::run(partitionTag, serverGroup, module_names, interface, partitionIndex, offline);
}
