#include "OperatorFactory.hh"

#include "ami/data/Single.hh"
#include "ami/data/Average.hh"
#include "ami/data/Integral.hh"
#include "ami/data/Reference.hh"
#include "ami/data/EntryMath.hh"
#include "ami/data/BinMath.hh"
#include "ami/data/EdgeFinder.hh"
#include "ami/data/PeakFinder.hh"
#include "ami/data/XYProjection.hh"
#include "ami/data/RPhiProjection.hh"
#include "ami/data/EnvPlot.hh"
#include "ami/data/Cds.hh"
#include "ami/data/Entry.hh"

#include <stdint.h>
#include <stdio.h>

using namespace Ami;

OperatorFactory::OperatorFactory(FeatureCache& f) :
  _f(f) {}

OperatorFactory::~OperatorFactory() {}

AbsOperator* OperatorFactory::_extract(const char*&     p, 
				       const DescEntry& input,
				       Cds&             output_cds) const
{
  uint32_t type = (AbsOperator::Type)*reinterpret_cast<const uint32_t*>(p);
  p+=sizeof(uint32_t);

  uint32_t next = *reinterpret_cast<const uint32_t*>(p);
  p+=sizeof(next);

  //  printf("OperatorFactory type %d\n",type);
  
  AbsOperator* o = 0;
  switch(type) {
  case AbsOperator::Single    : o = new Single   (p,input); break;
  case AbsOperator::Average   : o = new Average  (p,input); break;
  case AbsOperator::Mean      :
  case AbsOperator::Integral  : o = new Integral (p,input); break;
  case AbsOperator::Reference : o = new Reference(p,input); break;
  case AbsOperator::EntryMath : o = new EntryMath(p,input,output_cds); break;
  case AbsOperator::BinMath   : o = new BinMath  (p,input,_f); break;
  case AbsOperator::EdgeFinder: o = new EdgeFinder(p); break;
  case AbsOperator::PeakFinder: o = new PeakFinder(p, input); break;
  case AbsOperator::XYProjection  : o = new XYProjection  (p,input); break;
  case AbsOperator::RPhiProjection: o = new RPhiProjection(p,input); break;
  case AbsOperator::EnvPlot   : o = new EnvPlot(p,_f,output_cds); break;
  case AbsOperator::Value     :
  default: break;
  }
  if (next)
    o->next(_extract(p,o->output(),output_cds));
  return o;
}

AbsOperator* OperatorFactory::deserialize(const char*& p, 
					  const Entry& input,
					  Cds&         output_cds,
					  unsigned     output_signature) const
{
  AbsOperator* o = _extract(p,input.desc(),output_cds);
  return o;
}
