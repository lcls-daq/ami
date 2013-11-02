#include "ami/qt/XtcRun.hh"

#include "pdsdata/xtc/Dgram.hh"

#include <QtCore/QStringList>

#include <string>

//#define DBUG

using namespace Pds;

static const unsigned _buffer_size = 0x4000000;

static void* _alloc(Dgram& dg, unsigned size) {
  unsigned sz = dg.xtc.extent + dg.xtc.sizeofPayload() + size;
  if (sz > _buffer_size) {
    printf("Total event size (%d bytes) exceeds internal buffer size (%d bytes).  Aborting.\n",
           sz, _buffer_size);
    abort();
  }
  return dg.xtc.alloc(size);
}

static void _dump(const Sequence& seq, const char* title)
{
  printf("%s %08x.%08x:%05x [%s]\n",
         title,
         seq.clock().seconds(),
         seq.clock().nanoseconds(),
         seq.stamp().fiducials(),
         TransitionId::name(seq.service()));
}

Ami::Qt::XtcRun::XtcRun() : 
  _daqInit(false), 
  _buffer (new char[_buffer_size]) {}

Ami::Qt::XtcRun::~XtcRun()
{
  delete[] _buffer;
  for(std::list<Ana::XtcSlice*>::iterator it=_ioc.begin();
      it!=_ioc.end(); it++) 
    delete (*it);
}

void Ami::Qt::XtcRun::reset   (QStringList& files)
{
  _daqInit=false;
  while (! files.empty()) {
    std::string file = qPrintable(files.first());
    if (file.find("-s")!=std::string::npos &&
        file.find("-s8")==std::string::npos) {
      if (!_daqInit) {
	_daqInit=true;
	_daq.reset(file);
      }
      else
	_daq.add_file(file);
#ifdef DBUG
      printf("%s [DAQ]\n",file.c_str());
#endif
    }
    else {
      bool iocUsed=false;
      for(std::list<Ana::XtcSlice*>::iterator it=_ioc.begin();
	  it!=_ioc.end(); it++)
	if ((*it)->add_file(file)) {
	  iocUsed=true;
	  break;
	}
      if (!iocUsed)
	_ioc.push_back(new Ana::XtcSlice(file));
#ifdef DBUG
      printf("%s [IOC]\n",file.c_str());
#endif
    }
    files.pop_front();
  }
#ifdef DBUG
  printf("streams %d DAQ + %d IOC\n",
         _daqInit ? 1:0,
         _ioc.size());
#endif
}

void   Ami::Qt::XtcRun::init()
{
  _daq.init();
  for(std::list<Ana::XtcSlice*>::iterator it=_ioc.begin();
      it!=_ioc.end(); it++)
    (*it)->init();
}

Ana::Result Ami::Qt::XtcRun::next(Dgram*& dg)
{
  const unsigned FID_RANGE=720;  // 2 seconds
  const unsigned FID_APPROACH=Pds::TimeStamp::MaxFiducials-FID_RANGE;

  Ana::Result result = Ana::OK;

  Dgram* tdg;
  if (_daqInit) {
    //
    //  Use the DAQ event as the basis
    //
    result = _daq.next(tdg);

#ifdef DBUG
    _dump("DAQ entry",tdg->seq);
#endif

    if (_ioc.size()==0) {
      dg = tdg;
      return result;
    }

    if (result != Ana::Error) {
      dg = new(_buffer) Dgram;
      dg->seq = tdg->seq;
      dg->env = tdg->env;
      *new((char*)&dg->xtc) Xtc(TypeId(TypeId::Id_Xtc,0),
				ProcInfo(Level::Event,0,0));
      memcpy(_alloc(*dg,tdg->xtc.extent),&tdg->xtc,tdg->xtc.extent);
    }
  }
  else {
    //
    //  First, find the timestamp/fiducial of the next event
    //
    ClockTime tmin(-1,-1);
    unsigned       fmin(-1);
    std::list<Ana::XtcSlice*>::iterator n;

    for(std::list<Ana::XtcSlice*>::iterator it = _ioc.begin();
	it != _ioc.end(); it++) {
#ifdef DBUG
      printf("Consider [IOC %08x.%08x] %08x.%08x:%05x [%s]\n",
             (*it)->hdr().xtc.src.log(),
             (*it)->hdr().xtc.src.phy(),
             (*it)->hdr().seq.clock().seconds(),
             (*it)->hdr().seq.clock().nanoseconds(),
             (*it)->hdr().seq.stamp().fiducials(),
             TransitionId::name((*it)->hdr().seq.service()));
#endif
      const ClockTime& clk = (*it)->hdr().seq.clock();
      unsigned              fid = (*it)->hdr().seq.stamp().fiducials();
      if ((*it)->hdr().seq.service()==TransitionId::L1Accept) {
	if (fmin == -1U) {
	  n = it;
	  tmin = clk;
	  fmin = fid;
	}      
	else if (clk .seconds() <= tmin.seconds()+1 &&
		 tmin.seconds() <= clk .seconds()+1 &&
                 ((fmin  < FID_RANGE && fid > FID_APPROACH) ||
                  (fid   < fmin))) {
          n = it;
          tmin = clk;
          fmin = fid;
	}
	else if (clk.seconds() < tmin.seconds()) {
	  n = it;
	  tmin = clk;
	  fmin = fid;
	}
      }
    }
    if (fmin == -1U) { // non-L1A transition
      std::list<Ana::XtcSlice*>::iterator it = _ioc.begin();
      n = it;
      tmin = (*it)->hdr().seq.clock();
      fmin = (*it)->hdr().seq.stamp().fiducials();
    }

    dg = new(_buffer) Dgram;
    dg->seq = (*n)->hdr().seq;
    dg->env = (*n)->hdr().env;
    *new((char*)&dg->xtc) Xtc(TypeId(TypeId::Id_Xtc,0),
			      ProcInfo(Level::Event,0,0));

#ifdef DBUG
    _dump("IOC entry", dg->seq);
#endif
  }

  unsigned dgs = dg->seq.clock().seconds();
  unsigned dgf = dg->seq.stamp().fiducials();

  for(std::list<Ana::XtcSlice*>::iterator it=_ioc.begin();
      it!=_ioc.end(); ) {
    std::list<Ana::XtcSlice*>::iterator n = it++;
    unsigned iocs = (*n)->hdr().seq.clock().seconds();
    unsigned iocf = (*n)->hdr().seq.stamp().fiducials();
    if (dg->seq.isEvent()) {  // skip over earlier events
      while( (*n)->hdr().seq.isEvent() &&
             (iocs+1 < dgs ||
              (iocs  < dgs+2 &&
               ((dgf  < FID_RANGE && iocf > FID_APPROACH) ||
                (iocf < dgf)))) ) {
#ifdef DBUG
        _dump("skip IOC entry", (*n)->hdr().seq);
#endif
        if ((*n)->next(tdg)==Pds::Ana::End) break;
        iocs = (*n)->hdr().seq.clock().seconds();
        iocf = (*n)->hdr().seq.stamp().fiducials();
      }
    }
    else { // skip events until matching transition
      while( (*n)->hdr().seq.service() != dg->seq.service() ) {
#ifdef DBUG
        _dump("skip IOC entry", (*n)->hdr().seq);
#endif
        if ((*n)->next(tdg)==Pds::Ana::End) break;
        iocs = (*n)->hdr().seq.clock().seconds();
        iocf = (*n)->hdr().seq.stamp().fiducials();
      }
    }
      
    if ((!dg->seq.isEvent() && 
	 (*n)->hdr().seq.clock()==dg->seq.clock()) ||
	(dgf == iocf)) {
#ifdef DBUG
      _dump("add IOC entry", (*n)->hdr().seq);
#endif
      result = (*n)->next(tdg);
      memcpy(_alloc(*dg,tdg->xtc.extent),&tdg->xtc,tdg->xtc.extent);
      if (result == Ana::End ||
          tdg->seq.service()==TransitionId::EndRun) {
        result = Ana::End;
	delete *n;
	_ioc.erase(n);
      }
    }
#ifdef DBUG
    else
      _dump("ign IOC entry", (*n)->hdr().seq);
#endif
  }

#ifdef DBUG
  switch(result) {
  case Ana::Error: printf("Ana::Error\n"); break;
  case Ana::End  : printf("Ana::End\n"); break;
  case Ana::OK:
  default:
    break;
  }
#endif

  if (!_daqInit && result==Ana::End && _ioc.size())
    result=Ana::OK;
  
  return result;
}

int Ami::Qt::XtcRun::jump    (int calib, int jump, int& eventNum)
{
  if (_daqInit) return _daq.jump(calib,jump,eventNum);
  else          return 0;
}
      
int Ami::Qt::XtcRun::findTime(const char* sTime, int& iCalib, int& iEvent, bool& bExactMatch, bool& bOvertime)
{
  if (_daqInit) return _daq.findTime(sTime,iCalib,iEvent,bExactMatch,bOvertime);
  else          return 0;
}

int Ami::Qt::XtcRun::findTime(uint32_t uSeconds, uint32_t uNanoseconds, int& iCalib, int& iEvent, bool& bExactMatch, bool& bOvertime)
{
  if (_daqInit) return _daq.findTime(uSeconds,uNanoseconds,iCalib,iEvent,bExactMatch,bOvertime);
  else          return 0;
}

int Ami::Qt::XtcRun::getStartAndEndTime(ClockTime& start, ClockTime& end)
{
  if (_daqInit) return _daq.getStartAndEndTime(start,end);
  else          return 0;
}

int Ami::Qt::XtcRun::findNextFiducial
(uint32_t uFiducialSearch, int iFidFromEvent, int& iCalib, int& iEvent)
{
  if (_daqInit) return _daq.findNextFiducial(uFiducialSearch,iFidFromEvent,iCalib,iEvent);
  else          return 0;
}

int Ami::Qt::XtcRun::numTotalEvent(int& iNumTotalEvent)
{
  if (_daqInit) return _daq.numTotalEvent(iNumTotalEvent);
  else          return 0;
}

int Ami::Qt::XtcRun::numCalib(int& iNumCalib)
{
  if (_daqInit) return _daq.numCalib(iNumCalib);
  else          return 0;
}

int Ami::Qt::XtcRun::numEventInCalib(int calib, int& iNumEvents)
{
  if (_daqInit) return _daq.numEventInCalib(calib,iNumEvents);
  else          return 0;
}

int Ami::Qt::XtcRun::curEventId(int& eventGlobal, int& calib, int& eventInCalib)
{
  if (_daqInit) return _daq.curEventId(eventGlobal,calib, eventInCalib);
  else          return 0;
}

int Ami::Qt::XtcRun::nextCalibEventId(int iNumCalibAfter, int iNumEventAfter, bool bResetEventInCalib, int& eventGlobal, int& calib, int& eventInCalib)
{
  if (_daqInit) return _daq.nextCalibEventId(iNumCalibAfter, iNumEventAfter, bResetEventInCalib, eventGlobal, calib, eventInCalib);
  else          return 0;
}

void Ami::Qt::XtcRun::live_read(bool l)
{
  Pds::Ana::XtcRun::live_read(l);
}
