#include "ami/app/CmdLineTools.hh"

#include <stdlib.h>

using namespace Ami;

bool CmdLineTools::parseInt   (const char* arg, int& v)
{
  char* endptr;
  v = strtol(arg,&endptr,0);
  return *endptr==0;
}

bool CmdLineTools::parseUInt  (const char* arg, unsigned& v)
{
  char* endptr;
  v = strtoul(arg,&endptr,0);
  return *endptr==0;
}

bool CmdLineTools::parseUInt64(const char* arg, uint64_t& v)
{
  char* endptr;
  v = strtoull(arg,&endptr,0);
  return *endptr==0;
}

bool CmdLineTools::parseFloat (const char* arg, float& v)
{
  char* endptr;
  v = strtof(arg,&endptr);
  return *endptr==0;
}

bool CmdLineTools::parseDouble(const char* arg, double& v)
{
  char* endptr;
  v = strtod(arg,&endptr);
  return *endptr==0;
}

