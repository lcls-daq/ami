#include "ami/app/FilterExport.hh"
#include "ami/data/XML.hh"

#include "ami/data/AbsFilter.hh"
#include "ami/data/AbsOperator.hh"
#include "ami/data/CompoundFilter.hh"
#include "ami/data/FeatureRange.hh"
#include "ami/data/Analysis.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/FilterFactory.hh"
#include "ami/data/RawFilter.hh"
#include "ami/event/EventHandler.hh"
#include "ami/service/BuildStamp.hh"

#include <vector>
#include <string>
#include <sstream>
#include <fstream>

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

//#define DBUG

static const Pds::TypeId NoType(Pds::TypeId::Any,0);
static const unsigned MaxConfigSize = 64*1024;

using namespace Ami;
using namespace Pds;
using Ami::XML::QtPersistent;

static bool _lock  (std::string);
static void _unlock(std::string);
static void _insert_entry(char*& p, const Entry* e);
static void _insert_handler(char*& p, const EventHandler* h);
static void _insert_analysis(char*& p, const Analysis* a, bool discovery);
static void _insert_filter(char*& p, const AbsFilter* f);
static const char* _default_fname = "ami.l3t";

class CantFilter : public RawFilter {
public:
  CantFilter() {}
public:
  bool valid() const { return false; }
};

FilterImport::FilterImport(const std::string& input) :
  _status (false),
  _filter (new CantFilter)
{
  std::ostringstream o;

  if (input.size()==0) {
    o.str(std::string());
    o << "<QString name=\"error\">"
      << "Ami::L3T::FilterImport no input data"
      << "</QString>\n</Document>\n";
    _stream = o.str();
    perror(_stream.c_str());
    return;
  }
  
  o.str(std::string());
  o << input;
  o << "</Document>" << std::endl;

  _stream = o.str();

  FilterFactory fact;
  const char* p = _stream.c_str();
  XML_iterate_open(p,tag)
    if (tag.name == "L3TFilter") {
      XML_iterate_open(p,rtag)
        if      (rtag.name == "_filter") {
          XML_iterate_open(p,stag)
            if      (stag.name == "_filter") {
              delete _filter;
              const char* o = (const char*)QtPersistent::extract_op(p);
              _filter = fact.deserialize(o);
            }
	    else if (stag.name == "_text") {
	      QString s = QtPersistent::extract_s(p);
	      printf("Ami::FilterImport expr %s\n",qPrintable(s));
	    }
          XML_iterate_close(AbsFilter,stag);
        }
        else if (rtag.name == "_handlers")
          ;
        else if (rtag.name == "_analyses")
          ;
        else if (rtag.name == "tag")
          ;
        else if (rtag.name == "time")
          ;
      XML_iterate_close(FilterExport,rtag);
    }
  XML_iterate_close(FilterExport,tag);
  _status = true;
}

FilterImport::~FilterImport()
{
  if (_filter)
    delete _filter;
}

void FilterImport::parse_handlers(FilterImportCb& cb)
{
  const char* p = _stream.c_str();
  XML_iterate_open(p,tag)
    if (tag.name == "L3TFilter") {
      XML_iterate_open(p,rtag)
        if      (rtag.name == "_handlers") {
          unsigned log(-1U),phy(-1U);
          std::list<Pds::TypeId::Type> types;
          std::list<int> signatures, options;
          
          XML_iterate_open(p,stag)
            if      (stag.name == "_log")
              log      = QtPersistent::extract_i(p);
            else if (stag.name == "_phy")
              phy      = QtPersistent::extract_i(p);
            else if (stag.name == "_types")
              types.push_back(Pds::TypeId::Type(QtPersistent::extract_i(p)));
            else if (stag.name == "_entries") {
	      unsigned signature(-1U), options=0;
              XML_iterate_open(p,qtag)
               if (qtag.name == "signature")
                  signature = QtPersistent::extract_i(p);
                else if (qtag.name == "title")
                  ;
		else if (qtag.name == "options")
		  options = QtPersistent::extract_i(p);
              XML_iterate_close(Entry,qtag)
	      signatures.push_back((signature<<16)|(options&0xffff));
            }
          XML_iterate_close(EventHandler,stag);

          Src src;
          { uint32_t* d = reinterpret_cast<uint32_t*>(&src);
            d[0] = log;
            d[1] = phy; }
          cb.handler (src, 
                      types,
                      signatures);
        }
        else if (rtag.name == "_analyses")
          ;
        else if (rtag.name == "_filter")
          ;
        else if (rtag.name == "tag")
          ;
        else if (rtag.name == "time")
          ;
      XML_iterate_close(FilterExport,rtag);
    }
  XML_iterate_close(FilterExport,tag);
}

void FilterImport::parse_analyses(FilterImportCb& cb)
{
  const char* p = _stream.c_str();
  XML_iterate_open(p,tag)
    if (tag.name == "L3TFilter") {
      XML_iterate_open(p,rtag)
        if      (rtag.name == "_analyses") {
          unsigned id(-1U), input(-1U), output(-1U);
          bool     discovery=false;
          void*    op=0;
          
          XML_iterate_open(p,tag)
            if      (tag.name == "id")
              id       = QtPersistent::extract_i(p);
            else if (tag.name == "input")
              input    = QtPersistent::extract_i(p);
            else if (tag.name == "output")
              output   = QtPersistent::extract_i(p);
            else if (tag.name == "discovery")
              discovery = QtPersistent::extract_b(p);
            else if (tag.name == "operator")
              op     = QtPersistent::extract_op(p);
          XML_iterate_close(EventHandler,tag);

          cb.analysis(id,
                      discovery ? 
                      ConfigureRequest::Discovery :
                      ConfigureRequest::Analysis,
                      input,
                      output,
                      op);
        }
        else if (rtag.name == "_handlers")
          ;
        else if (rtag.name == "_filter")
          ;
        else if (rtag.name == "tag")
          ;
        else if (rtag.name == "time")
          ;
      XML_iterate_close(FilterExport,rtag);
    }
  XML_iterate_close(FilterExport,tag);
}

void FilterImport::parse_filter (FilterImportCb& cb)
{
  cb.filter(*_filter);
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
  std::stringstream o;
  if (fname==0 || strlen(fname)==0) {
    char* home = getenv("HOME");
    if (home)
      o << home << '/';
    o << _default_fname;
  }
  else
    o << fname;

  //
  //  Obtain a lock or silently fail
  //
  if (_lock(o.str())) {
    FILE* f = fopen(o.str().c_str(),"w");
    if (f) {
      char* buffer  = new char[MaxConfigSize];
      char* p = buffer;
      
      XML_insert(p, "FilterExport", "L3TFilter", _writex(p));
      
      fwrite(buffer, p-buffer, 1, f);
      
      delete[] buffer;
      fclose(f);
    }
    else
      printf("Failed to open [%s]\n",o.str().c_str());

    _unlock(o.str());
  }
  else {
    printf("Lock failed [%s]\n",o.str().c_str());
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
  
  XML_insert(p, "AbsFilter", "_filter", _insert_filter(p, _filter));
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
	  printf("Feature %s output from analysis %d:%d [%s]\n",
		 feature,(*it)->id(),(*it)->output().signature(),(*it)->output().name());
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
#ifdef DBUG
      printf("Feature %s not traced\n",feature);
#endif
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


void _insert_entry(char*& p, const Entry* e)
{
  XML_insert(p, "int", "signature", QtPersistent::insert(p, e->desc().signature()));
  XML_insert(p, "QString", "title", QtPersistent::insert(p, QString(e->desc().name())));
  XML_insert(p, "unsigned","options",QtPersistent::insert(p, e->desc().options()));
}

void _insert_handler(char*& p, const EventHandler* h)
{
  XML_insert(p, "unsigned", "_log", QtPersistent::insert(p, h->info().log()));
  XML_insert(p, "unsigned", "_phy", QtPersistent::insert(p, h->info().phy()));
  
  const std::list<Pds::TypeId::Type>& ctypes = h->config_types();
  for(std::list<Pds::TypeId::Type>::const_iterator it=ctypes.begin();
      it!=ctypes.end(); it++) {
    XML_insert(p,"unsigned", "_types", QtPersistent::insert(p, unsigned(*it)));
  }

  unsigned n = h->nentries();
  for(unsigned i=0; i<n; i++) {
    XML_insert(p, "Entry", "_entries", _insert_entry(p, h->entry(i)));
  }
}

void _insert_analysis(char*& p, const Analysis* a, bool discovery)
{
  RawFilter f;
  char* buff = new char[8*1024];
  XML_insert(p, "unsigned", "id"       , QtPersistent::insert(p, a->id()));
  XML_insert(p, "unsigned", "discovery", QtPersistent::insert(p, discovery));
  XML_insert(p, "unsigned", "input"    , QtPersistent::insert(p, a->input().desc().signature()));
  XML_insert(p, "unsigned", "output"   , QtPersistent::insert(p, a->output().signature()));
  XML_insert(p, "text"    , "text"     , QtPersistent::insert(p, QString(a->op().text().c_str())));
  XML_insert(p, "binary"  , "operator" , QtPersistent::insert(p, buff, (char*)a->op().serialize(f.serialize(buff))-buff));
}

void _insert_filter(char*& p, const AbsFilter* f)
{
  char* buffer = new char[8*1024];
  XML_insert(p, "text"  , "_text"  , QtPersistent::insert(p, QString(f->text().c_str())));
  XML_insert(p, "binary", "_filter", QtPersistent::insert(p, buffer, (char*)f->serialize(buffer)-buffer));
  delete[] buffer;
}

bool _lock(std::string fname)
{
  bool result=false;
  std::stringstream o;
  o << fname.c_str() << ".lock";
  std::string s_lock = o.str();

  o << "." << getenv("HOSTNAME") << "." << getpid();
  std::string s_unique = o.str();

  FILE* funique = fopen(s_unique.c_str(),"w");
  if (funique) {
    if (link(s_unique.c_str(), s_lock.c_str())==0)
      result = true;
    else {
      struct stat s;
      if (stat(s_unique.c_str(),&s)==0) {
	if (s.st_nlink==2)
	  result=true;
      }
      else
	perror("stat failed for unique file");
    }
    fclose(funique);
    if (!result && unlink(s_unique.c_str()))
      perror("unlink failed for unique");
  }
  else {
    o.str(std::string());
    o << "Failed to open funique " << s_unique.c_str();
    perror(o.str().c_str());
  }
  return result;
}

void _unlock(std::string fname)
{
  std::stringstream o;
  o << fname.c_str() << ".lock";
  std::string s_lock = o.str();
  if (unlink(s_lock.c_str()))
    perror("_unlock failed for lock");

  o << "." << getenv("HOSTNAME") << "." << getpid();
  std::string s_unique = o.str();
  if (unlink(s_unique.c_str()))
    perror("_unlock failed for unique");
}
