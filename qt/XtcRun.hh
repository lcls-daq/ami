#ifndef AmiQt_XtcRun_hh
#define AmiQt_XtcRun_hh

#include "pdsdata/ana/XtcRun.hh"

class QStringList;

namespace Ami {
  namespace Qt {
    class XtcRun {
    public:
      XtcRun();
      ~XtcRun();
    public:
      void reset   (QStringList&);
      
      void   init();
      Pds::Ana::Result next(Pds::Dgram*& dg);

      int jump    (int calib, int jump, int& eventNum);
      
      int findTime(const char* sTime, int& iCalib, int& iEvent, bool& bExactMatch, bool& bOvertime);
      int findTime(uint32_t uSeconds, uint32_t uNanoseconds, int& iCalib, int& iEvent, bool& bExactMatch, bool& bOvertime);
      int getStartAndEndTime(Pds::ClockTime& start, Pds::ClockTime& end);
      int findNextFiducial
      (uint32_t uFiducialSearch, int iFidFromEvent, int& iCalib, int& iEvent);

      int numTotalEvent(int& iNumTotalEvent);
      int numCalib(int& iNumCalib);
      int numEventInCalib(int calib, int& iNumEvents);  
      int curEventId(int& eventGlobal, int& calib, int& eventInCalib);

      int nextCalibEventId(int iNumCalibAfter, int iNumEventAfter, bool bResetEventInCalib, int& eventGlobal, int& calib, int& eventInCalib);

      void live_read(bool l);
    private:
      bool                           _daqInit;
      Pds::Ana::XtcRun               _daq;
      std::list<Pds::Ana::XtcSlice*> _ioc;
      char*                          _buffer;
    };
  };
};

#endif
