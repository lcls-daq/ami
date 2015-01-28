#include "ami/service/BuildStamp.hh"

const char* Ami::BuildStamp::time()
{
  return AMI_BUILD_TIME;
}

const char* Ami::BuildStamp::tag ()
{
  return AMI_BUILD_TAG;
}
