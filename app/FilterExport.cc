#include "ami/app/FilterExport.hh"
#include "ami/data/XML.hh"

#include "ami/data/AbsFilter.hh"
#include "ami/data/AbsOperator.hh"
#include "ami/data/CompoundFilter.hh"
#include "ami/data/FeatureRange.hh"
#include "ami/data/Analysis.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/event/EventHandler.hh"
#include "ami/service/BuildStamp.hh"

#define DBUG

using namespace Ami;
using Ami::XML::QtPersistent;

typedef std::list<const Analysis*>     AList;
typedef std::list<const EventHandler*> HList;


static void _insert_entry(char*& p, const Entry* e)
{
  XML_insert(p, "int", "signature", QtPersistent::insert(p, e->desc().signature()));
  XML_insert(p, "QString", "title", QtPersistent::insert(p, QString(e->desc().name())));
}

static void _insert_handler(char*& p, const EventHandler* h)
{
  XML_insert(p, "unsigned", "_log", QtPersistent::insert(p, h->info().log()));
  XML_insert(p, "unsigned", "_phy", QtPersistent::insert(p, h->info().phy()));

  unsigned n = h->nentries();
  XML_insert(p, "unsigned", "nentries", QtPersistent::insert(p, n));
  for(unsigned i=0; i<n; i++) {
    XML_insert(p, "Entry", "_entries", _insert_entry(p, h->entry(i)));
  }
}

static void _insert_analysis(char*& p, const Analysis* a, bool discovery)
{
  char* buff = new char[8*1024];
  XML_insert(p, "unsigned", "id"       , QtPersistent::insert(p, a->id()));
  XML_insert(p, "unsigned", "discovery", QtPersistent::insert(p, discovery));
  XML_insert(p, "unsigned", "input"    , QtPersistent::insert(p, a->input().desc().signature()));
  XML_insert(p, "unsigned", "output"   , QtPersistent::insert(p, a->output().signature()));
  XML_insert(p, "binary"  , "operator" , QtPersistent::insert(p, buff, (char*)a->op().serialize(buff)-buff));
}

static void _insert_filter(char*& p, const AbsFilter* f)
{
  char* buffer = new char[8*1024];
  XML_insert(p, "binary", "_filter", QtPersistent::insert(p, buffer, (char*)f->serialize(buffer)-buffer));
  delete[] buffer;
}



FilterExport::FilterExport(const char* fname)
{
  FILE* f = fopen(fname,"r");
  if (f) {
    //  Fill _sources and _analyses from XML file
    // ...
    fclose(f);
  }
  else {
    perror("FilterExport unable to open input");
  }
}

FilterExport::FilterExport(const AbsFilter& filter,
			   const HList& handlers,
			   const AList& analyses)
{
  _filter = filter.clone();

  _find_filter_sources(filter, handlers, analyses);

  AList an(_analyses);
  _analyses.clear();

  while(an.size()) {
    const Analysis* ai = an.front();
    an.remove(ai);
    const Analysis* ao = _find_analysis_source(ai,
					       handlers,
					       analyses);
    if (ao)
      an.push_back(ao);

    //  Dont allow duplicates
    _analyses.remove    (ai);
    _analyses.push_front(ai);
  }
}

FilterExport::~FilterExport() {}

void FilterExport::write(const char* fname) const
{
  FILE* f = fopen(fname,"w");
  if (f) {
    char* buffer  = new char[32*1024];
    char* p = buffer;

    XML_insert(p, "FilterExport", "L3TFilter", _writex(p));

    fwrite(buffer, p-buffer, 1, f);

    delete[] buffer;
    fclose(f);
  }
  else {
    perror("FilterExport::write: Unable to open output file");
  }
}

void FilterExport::_writex(char*& p) const
{
  XML_insert(p, "QString", "tag" , QtPersistent::insert(p, QString(BuildStamp::tag ())));
  XML_insert(p, "QString", "time", QtPersistent::insert(p, QString(BuildStamp::time())));

  for(HList::const_iterator it=_handlers.begin();
      it!=_handlers.end(); it++) {
    XML_insert(p, "EventHandler", "_handlers", _insert_handler(p,*it));
  }

  for(AList::const_iterator it=_analyses.begin();
      it!=_analyses.end(); it++) {
    XML_insert(p, "Analysis", "_analyses", 
	       _insert_analysis(p,*it,_from_discovery(*it)));
  }
  
  XML_insert(p, "AbsFilter", "filter", _insert_filter(p, _filter));
}

void FilterExport::_find_filter_sources(const AbsFilter& filter,
					const HList& handlers,
					const AList& analyses) 
{
  switch(filter.type()) {
  case AbsFilter::LogicAnd:
  case AbsFilter::LogicOr:
    { const CompoundFilter& f = static_cast<const CompoundFilter&>(filter);
      _find_filter_sources(f.a(), handlers, analyses);
      _find_filter_sources(f.b(), handlers, analyses);
    } break;
  case AbsFilter::FeatureRange:
    { const char* feature = static_cast<const FeatureRange&>(filter).feature();
      //  Find the source of this scalar
      for(AList::const_iterator it=analyses.begin();
	  it!=analyses.end(); it++)
	if ((*it)->output().type()==DescEntry::Cache &&
	    strcmp(feature, (*it)->output().name())==0) {
#ifdef DBUG
	  printf("Feature %s output from analysis %d:%d\n",
		 feature,(*it)->id(),(*it)->output().signature());
#endif
	  //  Dont allow duplicates
	  _analyses.remove    (*it);
	  _analyses.push_front(*it);
	  return;
	}
      for(HList::const_iterator it=handlers.begin();
	it!=handlers.end(); it++) {
	std::list<std::string> features((*it)->features());
	for(std::list<std::string>::const_iterator s=features.begin();
	    s!=features.end(); s++)
	  if (strcmp(s->c_str(),feature)==0) {
#ifdef DBUG
	    printf("Feature %s output from handler %08x.%08x\n",
		   feature,(*it)->info().log(),(*it)->info().phy());
#endif
	    _handlers.push_back(*it);
	    return;
	  }
      }
    } break;
  default:
    break;
  }
}

const Analysis*
FilterExport::_find_analysis_source(const Analysis* an,
				    const HList& handlers,
				    const AList& analyses) 
{
  //  Search through handlers' entries
  for(HList::const_iterator it=handlers.begin();
      it!=handlers.end(); it++) {
    unsigned n = (*it)->nentries();
    while(n--)
      if ((*it)->entry(n)==&an->input()) {
#ifdef DBUG
	printf("Found analysis %d:%d input handler %08x.%08x\n",
	       an->id(), an->output().signature(),
	       (*it)->info().log(), (*it)->info().phy());
#endif
	_handlers.push_back(*it);
	return 0;
      }
  }
  //  Search through analyses' outputs
  for(AList::const_iterator it=analyses.begin();
      it!=analyses.end(); it++)
    if ((*it)->id()==an->id() &&
	(*it)->output().signature()==an->input().desc().signature()) {
#ifdef DBUG
      printf("Found analysis %d:%d input analysis %d:%d\n",
	     an->id(), an->output().signature(),
	     (*it)->id(), (*it)->output().signature());
#endif
      return (*it);
    }
  return 0;
}

bool FilterExport::_from_discovery(const Analysis* a) const
{
  for(HList::const_iterator it=_handlers.begin(); it!=_handlers.end(); it++) {
    unsigned n = (*it)->nentries();
    for(unsigned i=0; i<n; i++) {
      const Entry* e = (*it)->entry(i);
      if (e == &a->input())
	return true;
    }
  }
  return false;
}
