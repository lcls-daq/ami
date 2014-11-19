#include "AbsOperator.hh"

#include <string.h>
#include <sstream>

using namespace Ami;

AbsOperator::AbsOperator(AbsOperator::Type t) : _type(t), _next(0) {}

AbsOperator::~AbsOperator() { if (_next) delete _next; }

AbsOperator::Type AbsOperator::type() const { return (AbsOperator::Type)_type; }

AbsOperator*      AbsOperator::next() const { return _next; }

void                   AbsOperator::next(AbsOperator* o) { _next=o; }

Entry&                 AbsOperator::operator()(const Entry& i) const
{
  Entry& o = _operate(i);
  return _next ? (*_next)(o) : o;
}

const DescEntry&       AbsOperator::output() const
{
  return _next ? _next->output() : _routput();
}

std::string            AbsOperator::text() const
{
  std::ostringstream s;
  s << _text();
  if (_next)
    s << " : " << _next->text();
  return s.str();
}

std::string            AbsOperator::_text() const { return std::string(); }

bool                   AbsOperator::valid() const
{
  if (!_valid()) return false;
  return _next ? _next->_valid() : true;
}

void                   AbsOperator::invalid()
{
  _invalid();
  if (_next)
    _next->invalid();
}

void*                  AbsOperator::serialize(void* p) const 
{ 
  static const uint32_t zero(0), one(1);
  _insert(p, &_type, sizeof(_type));
  _insert(p, _next ? &one : &zero, sizeof(uint32_t));
  p = _serialize(p);
  return _next ? _next->serialize(p) : p;
}

void AbsOperator::_insert(void*& p, const void* b, unsigned size) const 
{
  char* u = (char*)p;
  memcpy(u, b, size); 
  p = (void*)(u + size);
}

void AbsOperator::_extract(const char*& p, void* b, unsigned size) 
{
  memcpy(b, p, size); 
  p += size;
}

static const char* _type_str[] =  { "Single", "Average",
                                    "XYProjection", "RPhiProjection",
                                    "Reference", "EntryMath", "BinMath", "EdgeFinder", "PeakFinder",
                                    "EnvPlot", "PeakFitPlot", "FFT", "ContourProjection", "TdcPlot",
                                    "XYHistogram", "Zoom", "EntryRefOp", "Variance", "CurveFit", "MaskImage",
                                    "BlobFinder", "RectROI", "FIR", "Droplet", "LineFit", "Fit", NULL };


const char* AbsOperator::type_str(Type t) { return _type_str[t]; }
