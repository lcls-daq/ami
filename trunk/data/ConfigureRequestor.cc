#include "ami/data/ConfigureRequestor.hh"

#include "ami/data/ConfigureRequest.hh"

using namespace Ami;

ConfigureRequestor::ConfigureRequestor() :
  _prev_request(0),
  _changed     (true)
{
}

ConfigureRequestor::~ConfigureRequestor()
{
  if (_prev_request)
    delete[] _prev_request;
}

void ConfigureRequestor::request(ConfigureRequest&              req,
                                 unsigned&                      output,
                                 bool                           force)
{
//   printf("CR %p  prev %p  input %d  prev_output %d\n",
//          this, _prev_request, req.input(), 
//          _prev_request ? reinterpret_cast<ConfigureRequest*>(_prev_request)->output() : -1);
  const ConfigureRequest& preq = *reinterpret_cast<const ConfigureRequest*>(_prev_request);
  if (_prev_request && req == preq && preq.output()>=0 && !force) {
    req.output(reinterpret_cast<ConfigureRequest*>(_prev_request)->output());
    _changed=false;
  }
  else {
    if (_prev_request)
      delete[] _prev_request;

    req.output(++output);
    new (_prev_request = new char[req.size()]) ConfigureRequest(req);
    _changed=true;
  }
}
