#ifndef Ami_ConfigureRequest_hh
#define Ami_ConfigureRequest_hh

#include "ami/data/AbsFilter.hh"
#include "ami/data/AbsOperator.hh"

namespace Ami {

  class AbsFilter;
  class AbsOperator;

  class ConfigureRequest {
  public:
    enum State { Create, Destroy };
    ConfigureRequest(State        state, 
		     int          input,  // signature
		     int          output, // signature
		     const AbsFilter&   filter,
		     const AbsOperator& op) :
      _state(state), _input(input), _output(output)
    {
      char* e = (char*)op.serialize(filter.serialize(this+1));
      _size = e - (char*)this;
    }
  public:
    State state () const { return _state; }
    int   input () const { return _input; }
    int   output() const { return _output; }
    int   size  () const { return _size; }
  private:
    State    _state;
    int      _input;
    int      _output;
    int      _size;
  };

};

#endif
