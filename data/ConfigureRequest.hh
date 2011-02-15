#ifndef Ami_ConfigureRequest_hh
#define Ami_ConfigureRequest_hh

#include "ami/data/AbsFilter.hh"
#include "ami/data/AbsOperator.hh"

namespace Ami {

  class AbsFilter;
  class AbsOperator;

  class ConfigureRequest {
  public:
    enum State { Create, Destroy, SetOpt };
    enum Source { Discovery, Analysis, Summary, User, Hidden };
    ConfigureRequest(State        state, 
		     Source       source,
		     int          input,  // signature
		     int          output, // signature
		     const AbsFilter&   filter,
		     const AbsOperator& op) :
      _state(state), _source(source), _input(input), _output(output)
    {
      char* e = (char*)op.serialize(filter.serialize(this+1));
      _size = e - (char*)this;
    }
    ConfigureRequest(State        state,
		     Source       source) : 
      _state(state), _source(source), _input(-1), _output(-1), _size(sizeof(*this))
    {
    }
    ConfigureRequest(int          input,
                     unsigned     options) :
      _state (SetOpt),
      _source(Discovery),
      _input (input),
      _output(-1),
      _size  (sizeof(*this)+sizeof(options))
    {
      *reinterpret_cast<unsigned*>(this+1)=options;
    }
  public:
    State  state () const { return _state; }
    Source source() const { return _source; }
    int    input () const { return _input; }
    int    output() const { return _output; }
    int    size  () const { return _size; }
  private:
    State    _state;
    Source   _source;
    int      _input;
    int      _output;
    int      _size;
  };

};

#endif
