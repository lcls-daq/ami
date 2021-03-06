#include "ami/data/EntryFactory.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryTH2F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryProf2D.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryCache.hh"
#include "ami/data/EntryRef.hh"
#include "ami/data/EntryScalarRange.hh"
#include "ami/data/EntryScalarDRange.hh"

#include <stdio.h>
#include <stdlib.h>

using namespace Ami;

#define CASE_NEW(type) { case DescEntry::type: entry = new Entry##type((const Desc##type&)desc); break; }

Entry* EntryFactory::entry(const DescEntry& desc,
			   FeatureCache*    cache)
{
  Entry* entry = 0;
  switch (desc.type()) {
    CASE_NEW(Scalar);
    CASE_NEW(TH1F);
    CASE_NEW(TH2F);
    CASE_NEW(Prof);
    CASE_NEW(Prof2D);
    CASE_NEW(Scan);
    CASE_NEW(Image);
    CASE_NEW(Waveform);
    CASE_NEW(Ref);
    CASE_NEW(ScalarRange);
    CASE_NEW(ScalarDRange);
  case DescEntry::Cache: 
    { entry = new EntryCache((const DescCache&)desc, cache); break; }
  default: printf("EntryFactory::entry unrecognized type %d\n",desc.type()); abort(); break;
  }
  if (entry) entry->reset();
  return entry;
}
