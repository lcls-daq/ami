#ifndef Ami_ConfigureRequest_hh
#define Ami_ConfigureRequest_hh

#include "ami/data/AbsFilter.hh"
#include "ami/data/AbsOperator.hh"
#include "ami/data/FeatureCache.hh"

#include <string.h>
#include <string.h>

namespace Ami {

  class AbsFilter;
  class AbsOperator;

  class ConfigureRequest {
  public:
    enum State { Create, Destroy, SetOpt, Reset };
    enum Source { Discovery, Analysis, Summary, User, Filter, Hidden };
    ConfigureRequest(State        state, 
		     Source       source,
		     int          input,  // signature
		     int          output, // signature
		     const AbsFilter&   filter,
		     const AbsOperator& op,
                     ScalarSet    scalars =PreAnalysis) :
      _state(state), _source(source), _scalars(scalars), _input(input), _output(output)
    {
      char* e = (char*)op.serialize(filter.serialize(this+1));
      _size = e - (char*)this;
    }
    ConfigureRequest(State        state,
		     Source       source,
                     int          input) : 
      _state(state), _source(source), _scalars(PreAnalysis), _input(input), _output(-1), _size(sizeof(*this))
    {
    }
    ConfigureRequest(int          input,
                     unsigned     options) :
      _state (SetOpt),
      _source(Discovery),
      _scalars(PreAnalysis),
      _input (input),
      _output(-1),
      _size  (sizeof(*this)+sizeof(uint32_t))
    {
      *reinterpret_cast<uint32_t*>(this+1)=options;
    }
    ConfigureRequest(Source           source,
                     unsigned         options) :
      _state (SetOpt),
      _source(source),
      _scalars(PreAnalysis),
      _input (-1),
      _output(-1),
      _size  (sizeof(*this)+sizeof(uint32_t))
    {
      *reinterpret_cast<uint32_t*>(this+1)=options;
    }
    ConfigureRequest(Source           source,
                     unsigned         options,
                     const AbsFilter& filter,
		     const char*      output=0) :
      _state (SetOpt),
      _source(source),
      _scalars(PreAnalysis),
      _input (-1),
      _output(-1)
    {
      uint32_t* u = reinterpret_cast<uint32_t*>(this+1);
      *u=options;
      char* e = (char*)filter.serialize(u+1);
      if (output) {
	strcpy(e,output);
	e += strlen(output)+1;
      }
      else
	*e++ = 0;
     
      _size = e - (char*)this;
    }
    ConfigureRequest(const ConfigureRequest&);
    ~ConfigureRequest() {}
  public:
    bool  operator==(const ConfigureRequest&) const;
    void  output(unsigned s) { _output=s; }
  public:
    State  state () const { return State (_state); }
    Source source() const { return Source(_source); }
    ScalarSet scalars() const { return ScalarSet(_scalars); }
    int    input () const { return _input; }
    int    output() const { return _output; }
    int    size  () const { return _size; }
  private:
    uint32_t _state;
    uint32_t _source;
    uint32_t _scalars;
    int32_t  _input;
    int32_t  _output;
    int32_t  _size;
  };

};

#endif
