#include "ami/data/ConfigureRequest.hh"

//#define DBUG

using namespace Ami;

ConfigureRequest::ConfigureRequest(const ConfigureRequest& r)
{
  memcpy(this, &r, r.size());
}

bool ConfigureRequest::operator==(const ConfigureRequest& r) const
{
  //  Compare all fields except the output signature
  if (r.size()!=size()) {
#ifdef DBUG
    printf("ConfigureRequest::==(output %d) size differs\n",_output);
#endif
    return false;
  }
  //  return memcmp(this, &r, 4*sizeof(uint32_t))==0 && memcmp(this+1, &r+1, size()-sizeof(*this))==0;
  uint8_t* cthis = (uint8_t*)this;
  uint8_t* cthat = (uint8_t*)&r;
  for(unsigned i=0; i<4*sizeof(uint32_t); i++)
    if (cthis[i]!=cthat[i]) {
#ifdef DBUG
      printf("ConfigureRequest::==(output %d[%d]) this %02x that %02x %d\n",_output,r._output,unsigned(cthis[i]),unsigned(cthat[i]),i);
#endif
      return false;
    }
  for(unsigned i=sizeof(*this); i<unsigned(_size); i++)
    if (cthis[i]!=cthat[i]) {
#ifdef DBUG
      printf("ConfigureRequest::==(output %d[%d]) this %02x that %02x %d\n",_output,r._output,unsigned(cthis[i]),unsigned(cthat[i]),i);
#endif
      return false;
    }
  return true;
}
