#include "ami/app/FilterExport.hh"
#include "ami/data/AbsFilter.hh"
#include "ami/data/AbsOperator.hh"
#include "ami/data/ConfigureRequest.hh"
#include "pdsdata/xtc/Src.hh"

#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <getopt.h>

#include <fstream>
#include <sstream>

class MyCallback : public Ami::FilterImportCb {
public:
  MyCallback() {}
  void handler (const Pds::Src& src, 
                const std::list<Pds::TypeId::Type>& types,
                const std::list<int>& signatures)
  {
    printf("Handler: %08x.%08x\n",src.log(),src.phy());
    for(std::list<Pds::TypeId::Type>::const_iterator it=types.begin();
        it!=types.end(); it++)
      printf("\t%s\n",Pds::TypeId::name(*it));
    std::list<int>::const_iterator it=signatures.begin();
    printf("\t[%d",*it);
    while(++it!=signatures.end())
      printf(",%d",*it);
    printf("]\n");
  }
  void analysis(unsigned id, 
                Ami::ConfigureRequest::Source src,
                unsigned input, 
                unsigned output,
                void*    op) {
    printf("Analysis: %d %s[%d] %d %d\n",
           id, src==Ami::ConfigureRequest::Discovery ? "Discovery" : "Analysis",
           input, output, *reinterpret_cast<uint32_t*>(op));
  }
  void filter  (const Ami::AbsFilter& f) {
    printf("Filter: %d\n",f.type());
  }
};

static void usage(const char* p)
{
  printf("Usage: %s -f <filename> -h\n",p);
}

int main(int argc, char* argv[])
{
  const char* fname = "/tmp/filterexport.sav";

  int c;
  while ((c = getopt(argc, argv, "?hf:")) != -1) {
    switch(c) {
    case 'f':
      fname = optarg;
      break;
    case 'h':
    case '?':
    default:
      usage(argv[0]);
      return 0;
    }
  }

  std::ostringstream o;
  std::ifstream i(fname);
  if (i.good())
    i >> o.rdbuf();
  Ami::FilterImport e(o.str());

  MyCallback cb;
  e.parse_handlers(cb);
  e.parse_analyses(cb);
  e.parse_filter(cb);

  return 0;
}
