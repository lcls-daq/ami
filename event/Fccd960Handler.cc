#include "ami/event/Fccd960Handler.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "ami/event/FrameCalib.hh"
#include "ami/event/Calib.hh"
#include "ami/data/ChannelID.hh"
#include "ami/data/EntryImage.hh"
#include "pdsdata/psddl/camera.ddl.h"

//#define DBUG

#ifdef DBUG
static inline double tdiff(const timespec& tv_b, 
                           const timespec& tv_e)
{
  return double(tv_e.tv_sec-tv_b.tv_sec)+1.e-9*(double(tv_e.tv_nsec)-double(tv_b.tv_nsec));
}
#endif

using namespace Ami;

static const unsigned offset=1<<16;
static const double  doffset=double(offset);

static const Pds::TypeId Config_Type = Pds::TypeId(Pds::TypeId::Id_FccdConfig,2);
static const Pds::TypeId Data_Type   = Pds::TypeId(Pds::TypeId::Id_Frame,1);
static const unsigned Rows=960;
static const unsigned Columns=960;

static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_FrameFexConfig);
  types.push_back(Pds::TypeId::Id_PimImageConfig);
  types.push_back(Pds::TypeId::Id_FccdConfig);
  return types;
}

Fccd960Handler::Fccd960Handler(const Pds::Src& info, FeatureCache& cache) :
  EventHandler  (info, Data_Type.id(), config_type_list()),
  _cache        (cache),
  _entry        (0),
  _pentry       (0),
  _options      (0),
  _pedestals    (make_ndarray<unsigned>(1,1))
{
}

Fccd960Handler::~Fccd960Handler()
{
  if (_pentry)
    delete _pentry;
}

unsigned Fccd960Handler::nentries() const { return _entry ? 1 : 0; }

const Entry* Fccd960Handler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void Fccd960Handler::rename(const char* s)
{
  if (_entry)
    _entry->desc().name(s);
}

void Fccd960Handler::reset() 
{
  _entry = 0; 
  if (_pentry) { 
    delete _pentry; 
    _pentry=0; 
  }
}

void Fccd960Handler::_configure(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  if (tid.value() == Config_Type.value()) {
    const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());

    unsigned columns=Columns, rows=Rows;

    { int ppb = 1;
      DescImage desc(det, (unsigned)0, ChannelID::name(det),
		     columns, rows, ppb, ppb);
      _pentry = new EntryImage(desc);
      _pentry->invalid();
    }

    int ppb = image_ppbin(columns,rows);
    DescImage desc(det, (unsigned)0, ChannelID::name(det),
                   columns, rows, ppb, ppb);
    _entry = new EntryImage(desc);
    _entry->invalid();

    _load_pedestals();
    _load_gains    ();

    _cm_channel = make_ndarray<unsigned>(2,96,2);
    for(unsigned* p=_cm_channel.begin(); p!=_cm_channel.end(); ) {
      *p++ = offset-32;
      *p++ = offset+32;
    }
    _cm_row     = make_ndarray<unsigned>(960,12,2);
    for(unsigned* p=_cm_row.begin(); p!=_cm_row.end(); ) {
      *p++ = offset-32;
      *p++ = offset+32;
    }
  }
}

void Fccd960Handler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

bool Fccd960Handler::used() const 
{ 
  if (_entry && _entry->desc().used()) return true;
  return false;
}

void Fccd960Handler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  if (_entry && _entry->desc().used()) {
    unsigned o = _entry->desc().options();
    if (_options != o) {
      printf("Fccd960Handler::event options %x -> %x\n", _options, o);
      _options = o;
    }

    if (_entry->desc().options() & FrameCalib::option_reload_pedestal()) {
      _load_pedestals();
      _entry->desc().options( _entry->desc().options()&~FrameCalib::option_reload_pedestal() );
    }

    const Pds::Camera::FrameV1& f = *reinterpret_cast<const Pds::Camera::FrameV1*>(payload);

    _entry->reset();
    const DescImage& d = _entry->desc();

    const ndarray<const unsigned,2>& p =
      d.options()&FrameCalib::option_no_pedestal() ?
      _offset : _pedestals;

    ndarray<const uint16_t,2> a = f.data16();

#ifdef DBUG
    timespec tv_b,tv_e;
    clock_gettime(CLOCK_REALTIME,&tv_b);
#endif
    ndarray<unsigned,2> e = make_ndarray<unsigned>(a.shape()[0],a.shape()[1]);
    { const uint16_t* pa=a.begin();
      const unsigned* pp=p.begin();
      for(unsigned* pe=e.begin(); pe!=e.end(); pe++,pa++,pp++)
        *pe = (*pa&0x1fff)+*pp;
    }
#ifdef DBUG
    clock_gettime(CLOCK_REALTIME,&tv_e);
    printf("Fccd960H:apply_ped %f sec\n",tdiff(tv_b,tv_e));
#endif

    //
    //  correct each line even(odd) pixels in blocks of 160 columns
    //
    if (d.options()&FrameCalib::option_correct_common_mode2()) {
#ifdef DBUG
      clock_gettime(CLOCK_REALTIME,&tv_b);
#endif
      const int str[] = {2};
      const unsigned asicCol = 160;
      for(unsigned i=0; i<e.shape()[0]; i++) {
        for(unsigned j=0; j<e.shape()[1]/asicCol; j++)
          for(unsigned k=0; k<2; k++) {
            ndarray<uint32_t,1> s = make_ndarray<uint32_t>(&e[i][j*asicCol+k],asicCol/2);
            s.strides(str);
            unsigned m=2*j+k;
            int fn = FrameCalib::median(s,
                                        _cm_row[i][m][0],
                                        _cm_row[i][m][1]);
            if (fn<0) {
              printf("Fcc960Handler common_mode2 skipping row %d col %d\n",
                     i,m);
              _cm_row[i][m][0] = offset-32;
              _cm_row[i][m][1] = offset+32;
              fn = 0;
            }
            else
              fn-=offset;
            uint32_t* v = &s[0];
            for(unsigned x=0; x<asicCol; x+=2)
              v[x] -= fn;
          }
      }
#ifdef DBUG
      clock_gettime(CLOCK_REALTIME,&tv_e);
      printf("Fccd960H:apply_cm2 %f sec\n",tdiff(tv_b,tv_e));
#endif
    }

    //
    //  correct each block (10 columns x 480 rows)
    //
    if (d.options()&FrameCalib::option_correct_common_mode()) {
#ifdef DBUG
      clock_gettime(CLOCK_REALTIME,&tv_b);
#endif
      const unsigned asicRow = 480;
      const unsigned asicCol =  10;
      for(unsigned i=0; i<Rows/asicRow; i++)
        for(unsigned j=0; j<Columns/asicCol; j++) {
          ndarray<uint32_t,2> s = make_ndarray<uint32_t>(&e[i*asicRow][j*asicCol],asicRow,asicCol);
          s.strides(e.strides());
          int fn = FrameCalib::median(s,
                                      _cm_channel[i][j][0],
                                      _cm_channel[i][j][1])-offset;
          for(unsigned y=0; y<asicRow; y++) {
            uint32_t* v = &s[y][0];
            for(unsigned x=0; x<asicCol; x++)
              v[x] -= fn;
          }
        }          
#ifdef DBUG
      clock_gettime(CLOCK_REALTIME,&tv_e);
      printf("Fccd960H:apply_cm  %f sec\n",tdiff(tv_b,tv_e));
#endif
    }

    //
    //  Correct for gain
    //
    if (d.options()&FrameCalib::option_correct_gain()) {
#ifdef DBUG
      clock_gettime(CLOCK_REALTIME,&tv_b);
#endif
      const double* pg=_gain.begin();
      for(unsigned* pe=e.begin(); pe!=e.end(); pe++,pg++)
        *pe = unsigned(*pg*(double(int(*pe-offset))))+offset;
#ifdef DBUG
      clock_gettime(CLOCK_REALTIME,&tv_e);
      printf("Fccd960H:apply_gain %f sec\n",tdiff(tv_b,tv_e));
#endif
    }

    //
    //  Bin
    //
#ifdef DBUG
    clock_gettime(CLOCK_REALTIME,&tv_b);
#endif
    const int ppbin = _entry->desc().ppxbin();
    switch(ppbin) {
    case 1:
      { unsigned* enp = _entry->contents();
        for(const unsigned* pe=e.begin(); pe!=e.end(); pe++,enp++)
          *enp = *pe;
      } break;
    case 2:
      { unsigned* enp = _entry->contents();
        for(unsigned i=0; i<Rows; i+=2) {
          const unsigned* pe =&e[i][0];
          const unsigned* pe2=&e[i+1][0];
          for(unsigned j=0; j<Columns; j+=2)
            *enp++ = pe[j+0]+pe[j+1]+pe2[j+0]+pe2[j+1];
        }
      } break;
    default:
      for(unsigned i=0; i<Rows; i++)
        for(unsigned j=0; j<Columns; j++)
          _entry->addcontent(e[i][j],i/ppbin,j/ppbin);
      break;
    }
#ifdef DBUG
    clock_gettime(CLOCK_REALTIME,&tv_e);
    printf("Fccd960H:apply_bin %f sec\n",tdiff(tv_b,tv_e));
#endif

    _entry->info(double(offset*d.ppxbin()*d.ppybin()),EntryImage::Pedestal);
    _entry->info(1.,EntryImage::Normalization);
    _entry->valid(t);
  }
}

void Fccd960Handler::_damaged() { if (_entry) _entry->invalid(); }

void Fccd960Handler::_load_pedestals()
{
  _pedestals    = make_ndarray<unsigned>(Rows,Columns);
  _offset       = make_ndarray<unsigned>(Rows,Columns);
  for(unsigned* a = _offset.begin(); a!=_offset.end(); *a++ = offset) ;

  EntryImage* p = _pentry;
  if (FrameCalib::load_pedestals(p,offset,"ped")) {
    for(unsigned *a=_pedestals.begin(), *b=p->contents(); a!=_pedestals.end(); *a++=*b++) ;
  }
  else
    for(unsigned* a=_pedestals.begin(); a!=_pedestals.end(); *a++=offset) ;
}

void Fccd960Handler::_load_gains()
{
  _gain    = make_ndarray<double>(Rows,Columns);
  _no_gain = make_ndarray<double>(Rows,Columns);

  for(double* a=_no_gain.begin(); a!=_no_gain.end(); *a++=1.) ;

  EntryImage* p = _pentry;
  ndarray<double,3> ap = FrameCalib::load(p->desc(),"gain");
  if (ap.size())
    for(double *a=_gain.begin(), *b=ap.begin(); a!=_gain.end(); *a++=*b++) ;
  else
    for(double* a=_gain.begin(); a!=_gain.end(); *a++=1.) ;
}


