#include "Calib.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <sstream>
#include <sys/stat.h>
#include <glob.h>
#include <libgen.h>
#include <iomanip>
#include <vector>

static std::string onlRoot("/reg/g/pcds/pds/");
static std::string offRoot("/reg/d/psdm/");
static std::string _expt;
static int         _run =0;
static bool        _use_offline=false;
static bool        _use_test   =false;

void Ami::Calib::set_experiment(const char* e) { _expt = std::string(e); }
void Ami::Calib::set_run       (int r)         { _run  = r; }

void Ami::Calib::use_offline(bool v) { _use_offline=v; }
void Ami::Calib::use_test   (bool v) { _use_test=v; }

bool Ami::Calib::use_test   () { return _use_test; }


static const char* onlCalibClass[] = { NULL,                // NoDevice
                                       NULL,                // EVR
                                       "acqcalib",          // Acqiris
                                       "calib",             // Opal1k
                                       "calib",             // TM6740
                                       "pnccdcalib",        // pnCCD
                                       "calib",             // Princeton
                                       "fccdcalib",         // FCCD
                                       NULL,
                                       NULL,
                                       "cspadcalib",        // Cspad
                                       NULL,                // AcqTDC
                                       NULL,                // Xamps
                                       "cspadcalib",        // Cspad2x2
                                       NULL };

static const char* offCalibClass[] = { NULL,                // NoDevice
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       "CsPad::CalibV1",
                                       NULL,
                                       NULL,
                                       "CsPad2x2::CalibV1",
                                       NULL };

static FILE* _fopen(const char* fname)
{
  printf("Calib::fopen opening %s\n",fname);
  return ::fopen(fname,"r");
}

FILE* Ami::Calib::fopen(const Pds::DetInfo& info,
                        const char* onl_calib_type,
                        const char* off_calib_type)
{
  std::string path1;
  { std::ostringstream o;
    o << onl_calib_type << "."
      << std::hex << std::setw(8) << std::setfill('0') << info.phy()
      << ".dat";
    path1 = o.str(); }

  std::string path2;
  if (onlCalibClass[info.device()]) {
    std::ostringstream o;
    o << onlRoot << onlCalibClass[info.device()] << "/" << path1;
    path2 = o.str(); 
  }

  std::string path3;
  if (!_expt.empty() && offCalibClass[info.device()]) {
    std::string hutch = _expt.substr(0,3);
    for(unsigned i=0; i<hutch.size(); i++)
      hutch[i] = toupper(hutch[i]);

    std::ostringstream o;
    o << offRoot << hutch << "/" << _expt << "/calib/"
      << offCalibClass[info.device()] << "/"
      << Pds::DetInfo::name(info.detector()) << "." << info.detId() << ":" 
      << Pds::DetInfo::name(info.device  ()) << "." << info.devId()
      << "/" << off_calib_type << "/*";

    char buff[256];
    glob_t g;
    glob(o.str().c_str(),0,0,&g);
    for(unsigned i=0; i<g.gl_pathc; i++) {
      std::string base(basename(strcpy(buff,g.gl_pathv[i])));
      std::string::size_type p = base.find("-");
      if (p == std::string::npos)
        continue;
  
      std::string::size_type q = base.find(".");
      if (q != std::string::npos)
        q -= p+1;

      std::string beginstr(base, 0, p);
      std::string endstr  (base, p+1, q);

      char* endPtr;
      int begin = strtol(beginstr.c_str(),&endPtr,0);
      if (endPtr==beginstr.c_str() || begin > _run)
        continue;

      if (endstr != "end") {
        int end = strtol(endstr.c_str(),&endPtr,0);
        if (endPtr==endstr.c_str() || end < _run)
          continue;
      }
      path3 = std::string(g.gl_pathv[i]);
      break;
    }
  }

  //  Try to get NFS client cache coherency
  char path[256];
  struct stat64 st1,st2,st3;
  stat64(dirname(strcpy(path,path1.c_str())),&st1);
  stat64(dirname(strcpy(path,path2.c_str())),&st1);
  memset(&st1,0,sizeof(st1));
  memset(&st2,0,sizeof(st2));
  memset(&st3,0,sizeof(st3));
  if (!path1.empty()) stat64(path1.c_str(),&st1);
  if (!path2.empty()) stat64(path2.c_str(),&st2);
  if (!path3.empty()) stat64(path3.c_str(),&st3);

  if (st1.st_mtime > 0)
    return _fopen(path1.c_str());
  if (_use_offline && st3.st_mtime > 0)
    return _fopen(path3.c_str());
  if (st2.st_mtime > 0)
    return _fopen(path2.c_str());

  printf("Calib::fopen failed to open [%s,%s,%s]\n",
         path1.c_str(),path2.c_str(),path3.c_str());

  return 0;
}

FILE* Ami::Calib::fopen_dual(const char *path1, const char * path2, const char *description)
{
  //  Try this to get client cache coherency
  char path[256];
  strcpy(path,path1);
  struct stat64 st;
  stat64(dirname(path),&st);

  const int ErrMsgSize=200;
  char errmsg[ErrMsgSize];
  FILE *f = ::fopen(path1, "r");

  if (f) {
    printf("Loaded %s from %s\n", description, path1);
  } else {
    snprintf(errmsg, ErrMsgSize, "fopen: Failed to load %s from %s", description, path1);
    perror(errmsg);

    strcpy(path,path2);
    stat64(dirname(path),&st);
    f = ::fopen(path2, "r");
    if (f) {
      printf("Loaded %s from %s\n", description, path2);
    } else {
      snprintf(errmsg, ErrMsgSize, "fopen: Failed to load %s from %s", description, path2);
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
