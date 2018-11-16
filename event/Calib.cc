#include "Calib.hh"
#include "CalibFile.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <sstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <glob.h>
#include <libgen.h>
#include <string.h>
#include <iomanip>
#include <vector>

#define DBUG

static std::string onlRoot("/reg/g/pcds/pds/");
static std::string offRoot("/reg/d/psdm/");
static std::string _expt;
static unsigned    _run =0x7fffffff;
static bool        _use_offline=false;
static bool        _use_online =true;
static bool        _use_test   =false;
static bool        _show_write_pedestals=false;

static std::string offl_path(std::string basepath, 
                             const Pds::DetInfo& info,
                             std::string off_calib_type);

void Ami::Calib::set_experiment(const char* e) { _expt = std::string(e); }
void Ami::Calib::set_run       (int r)         { _run  = r; }

void Ami::Calib::use_offline(bool v) { _use_offline=v; }
void Ami::Calib::use_online (bool v) { _use_online=v; }
void Ami::Calib::use_test   (bool v) { _use_test=v; }

bool Ami::Calib::use_offline() { return _use_offline; }
bool Ami::Calib::use_online () { return _use_online; }
bool Ami::Calib::use_test   () { return _use_test; }

void Ami::Calib::show_write_pedestals(bool v) { _show_write_pedestals=v; }
bool Ami::Calib::show_write_pedestals() { return _show_write_pedestals; }


static const char* onlCalibClass = "calib";

struct off_lookup {
  Pds::DetInfo::Device t;
  std::string          value;
};

static struct off_lookup _off_lookup[] = { {Pds::DetInfo::pnCCD,    "PNCCD::CalibV1"},
                                           {Pds::DetInfo::Cspad,    "CsPad::CalibV1"},
                                           {Pds::DetInfo::Cspad2x2, "CsPad2x2::CalibV1"},
                                           {Pds::DetInfo::Epix10k,  "Epix10k::CalibV1"},
                                           {Pds::DetInfo::Epix10ka, "Epix10ka::CalibV1"},
                                           {Pds::DetInfo::Epix100a, "Epix100a::CalibV1"},
                                           {Pds::DetInfo::Epix10ka, "Epix10ka::CalibV1"},
                                           {Pds::DetInfo::Fccd960,  "Camera::CalibV1"},
                                           {Pds::DetInfo::Zyla,     "Camera::CalibV1"},
                                           {Pds::DetInfo::Uxi,      "Uxi::CalibV1"},
                                           {Pds::DetInfo::Archon,   "Camera::CalibV1"},
                                           {Pds::DetInfo::Jungfrau, "Jungfrau::CalibV1"} };

static const char* offCalibClass(Pds::DetInfo::Device t)
{
  for(unsigned i=0; i<sizeof(_off_lookup)/sizeof(struct off_lookup); i++)
    if (_off_lookup[i].t == t)
      return _off_lookup[i].value.c_str();
  return "";
}

static FILE* _fopen(const char* fname, bool no_cache)
{
  printf("Calib::fopen opening %s\n",fname);

  FILE* f = 0;
  int flags = no_cache ? O_RDONLY|O_DIRECT : O_RDONLY;
  int fd = ::open(fname,flags);
  if (fd >= 0)
    f = ::fdopen(fd,"r");
    
  return f;
}

static void _stat64(const char* fname, struct stat64* s, bool no_cache)
{
  if (no_cache) {
    int fd = ::open(fname,O_RDONLY|O_DIRECT);
    stat64(fname,s);
    if (fd >= 0)
      ::close(fd);
  }
  else
    stat64(fname,s);
}

int Ami::Calib::get_line(char** p, size_t* n, FILE* f) {
  int v=0;
  while( (v=getline(p, n, f))==1 || (*p)[0]=='#') ;
  return v;
}

void Ami::Calib::skip_header(FILE* f) {
  size_t sz=8*1024;
  char* linep = new char[sz];
  long v = get_line(&linep, &sz, f);
  delete[] linep;
  fseek(f, -v, SEEK_CUR);
}

FILE* Ami::Calib::fopen(const Pds::DetInfo& info,
                        const char* onl_calib_type,
                        const char* off_calib_type,
                        bool no_cache,
                        bool* offl_type,
			std::string* fname)
{
  std::string path1;
  { std::ostringstream o;
    o << onl_calib_type << "."
      << std::hex << std::setw(8) << std::setfill('0') << info.phy()
      << ".dat";
    path1 = o.str(); }

  std::string path2;
  {
    std::ostringstream o;
    o << onlRoot << onlCalibClass << "/" << path1;
    path2 = o.str(); 
  }

  std::string path3;
  if (!_expt.empty() && offCalibClass(info.device())) {
    std::string hutch = _expt.substr(0,3);
    for(unsigned i=0; i<hutch.size(); i++)
      hutch[i] = toupper(hutch[i]);

    std::ostringstream o;
    o << offRoot << hutch << "/" << _expt << "/calib/";
    path3 = offl_path(o.str(),info,off_calib_type);
  }

  std::string path4;
  if (!_expt.empty() && offCalibClass(info.device())) {
    path4 = offl_path(std::string("./calib/"),info,off_calib_type);
  }

#ifdef DBUG
  printf("Trying paths [%s] [%s] [%s] [%s] (%s) (%s)\n",
         path1.c_str(), path2.c_str(), path3.c_str(), path4.c_str(), _expt.c_str(), offCalibClass(info.device()));
#endif

  //  Try to get NFS client cache coherency
  char path[256];
  struct stat64 st1,st2,st3,st4;
  stat64(dirname(strcpy(path,path1.c_str())),&st1);
  stat64(dirname(strcpy(path,path2.c_str())),&st1);
  memset(&st1,0,sizeof(st1));
  memset(&st2,0,sizeof(st2));
  memset(&st3,0,sizeof(st3));
  memset(&st4,0,sizeof(st4));
  if (!path1.empty()) _stat64(path1.c_str(),&st1,no_cache);
  if (!path2.empty()) _stat64(path2.c_str(),&st2,no_cache);
  if (!path3.empty()) _stat64(path3.c_str(),&st3,no_cache);
  if (!path4.empty()) _stat64(path4.c_str(),&st4,no_cache);

  if (_use_offline && st4.st_mtime > 0) {
    if (offl_type) *offl_type=true;
    if (fname) *fname=path4;
    return _fopen(path4.c_str(),no_cache);
  }
  if (_use_online && st1.st_mtime > 0) {
    if (offl_type) *offl_type=false;
    if (fname) *fname=path1;
    return _fopen(path1.c_str(),no_cache);
  }
  if (_use_offline && st3.st_mtime > 0) {
    if (offl_type) *offl_type=true;
    if (fname) *fname=path3;
    return _fopen(path3.c_str(),no_cache);
  }
  if (_use_online && st2.st_mtime > 0) {
    if (offl_type) *offl_type=false;
    if (fname) *fname=path2;
    return _fopen(path2.c_str(),no_cache);
  }

  printf("Calib::fopen failed to open [%s,%s,%s]\n",
         path1.c_str(),path2.c_str(),path3.c_str());

  return 0;
}

FILE* Ami::Calib::fopen_dual(const char *path1, const char * path2, 
                             const char *description,
                             bool        no_cache)
{
  FILE *f = 0;
  const int ErrMsgSize=200;
  char errmsg[ErrMsgSize];
  int flags = no_cache ? O_RDONLY|O_DIRECT : O_RDONLY;
  int fd = ::open(path1, flags);
  struct stat st;

  if (fd >= 0) {
    f = ::fdopen(fd, "r");
    int rst=fstat(fd, &st);
    printf("Loaded %s from %s [%s (%d)]\n", description, path1,ctime(&st.st_mtime),rst);
  } else {
    fd = ::open(path2, flags);
    if (fd >= 0) {
      f = ::fdopen(fd, "r");
      int rst=fstat(fd, &st);
      printf("Loaded %s from %s [%s (%d)]\n", description, path2,ctime(&st.st_mtime),rst);
    } else {
      snprintf(errmsg, ErrMsgSize, "fopen: Failed to load %s from %s or %s", description, path1, path2);
      perror(errmsg);
    }
  }
  return (f);
}

void Ami::Calib::load_array(ndarray<double,1>& a, unsigned phy, 
                            const char* pfx, const char* dsc)
{
  char path1[128], path2[128];
  sprintf(path1,"%s.%08x.dat",pfx,phy);
  sprintf(path2,"/reg/g/pcds/pds/calib/%s.%08x.dat",pfx,phy);
  FILE* f = Calib::fopen_dual(path1, path2, dsc);

  std::vector<double> va;
  if (f) {
    float v;
    while(!feof(f)) {
      if (fscanf(f,"%f",&v)==1)
        va.push_back(v);
    }
  }
  a = make_ndarray<double>(va.size());
  for(unsigned i=0; i<va.size(); i++)
    a[i] = va[i];
}

void Ami::Calib::load_integral_symm(ndarray<double,1>& a, 
				    unsigned phy,
				    const char* pfx, const char* dsc)
{
  char path1[128], path2[128];
  sprintf(path1,"%s.%08x.dat",pfx,phy);
  sprintf(path2,"/reg/g/pcds/pds/calib/%s.%08x.dat",pfx,phy);
  FILE* f = Calib::fopen_dual(path1, path2, dsc);

  std::vector<double> va;
  if (f) {
    float v,x;
    while(!feof(f)) {
      if (fscanf(f,"%f %f",&x,&v)==2)
        va.push_back(v);
    }
  }

  a = make_ndarray<double>(va.size());
  if (va.size()) {
    a[0] = va[0]+va[va.size()-1];
    for(unsigned i=1; i<va.size(); i++)
      a[i] = a[i-1]+va[i]+va[va.size()-i-1];
    double n = a[va.size()-1];
    for(unsigned i=va.size()-1; i>0; i--)
      a[i] = 0.5*(a[i-1]+a[i])/n;
    a[0] *= 0.5;
#if 0
    for(unsigned i=0; i<=va.size(); i++)
      printf("%f%c",a[i], (i%10==9) ? '\n':' ');
    printf("\n");
#endif
  }
}

std::string offl_path(std::string basepath, 
                      const Pds::DetInfo& info,
                      std::string off_calib_type)
{
  std::ostringstream o;
  o << basepath
    << offCalibClass(info.device()) << "/"
    << Pds::DetInfo::name(info.detector()) << "." << info.detId() << ":" 
    << Pds::DetInfo::name(info.device  ()) << "." << info.devId()
    << "/" << off_calib_type << "/*.data";

  //
  //  Follow the CalibFileFinder.cpp prescription
  //
  glob_t g;
  glob(o.str().c_str(),0,0,&g);

  std::vector<Ami::CalibFile> calfiles;

  for(unsigned i=0; i<g.gl_pathc; i++) {
    try {
      Ami::CalibFile f(std::string(g.gl_pathv[i]));
      calfiles.push_back(f);
    } 
    catch (const std::exception& ex) {}
    catch (const std::string& ex) {}
  }
  globfree(&g);

  std::sort(calfiles.begin(), calfiles.end());
  typedef std::vector<Ami::CalibFile>::const_reverse_iterator FileIter;
  for (FileIter it = calfiles.rbegin() ; it != calfiles.rend() ; ++ it ) {
    if (it->begin() <= _run &&
	_run <= it->end())
      return it->path();
  }
  return std::string();
}

Ami::CalibIO::CalibIO(FILE& f) :
  _f(f),
  _sz(8*1024),
  _linep((char*)malloc(_sz))
{
  memset(_linep, 0, _sz);
}

Ami::CalibIO::~CalibIO()
{
  free(_linep);
}

bool Ami::CalibIO::next_line()
{
  while(1) {
    if (getline(&_linep, &_sz, &_f)<0)
      return false;
    if (_linep[0]=='#' || _linep[0]=='\n') continue;
    _pEnd=_linep;
    break;
  }
  return true;
}

char* Ami::CalibIO::line()
{
  return _linep;
}

unsigned Ami::CalibIO::getul()
{
  _ppEnd=_pEnd;
  return strtoul(_pEnd,&_pEnd,0);
}

double Ami::CalibIO::getdb()
{
  _ppEnd=_pEnd;
  return strtod (_pEnd,&_pEnd);
}

bool Ami::CalibIO::get_failed() const
{
  return _pEnd==_ppEnd;
}
