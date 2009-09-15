#include "Port.hh"

using namespace Ami;

unsigned short Port::clientPort()     { return 5720; }
unsigned short Port::serverPortBase() { return 5721; }
unsigned short Port::nServerPorts() { return 10; }
