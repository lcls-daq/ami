#ifndef SummaryAnalysis_hh
#define SummaryAnalysis_hh

#include "pdsdata/acqiris/ConfigV1.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "ami/app/SyncAnalysis.hh"
#include "ami/data/EntryTH1F.hh"
#include "pdsdata/evr/DataV3.hh" 

#include <list>

namespace Pds {
  class ClockTime;
  class Src;
  class TypeId;
};

namespace Ami {
  class Cds;
  class SyncAnalysis;
  class EntryTH1F;

  class SummaryAnalysis {
  public:
    enum {ForceRefill, ValidateRefill};
    static SummaryAnalysis& instance();
  private:
    SummaryAnalysis();
    ~SummaryAnalysis();
  public:  // Handler functions
    void reset    ();
    void clock    (const Pds::ClockTime& clk);
    void configure(const Pds::Src& src, const Pds::TypeId& type, void* payload);
    void event    (const Pds::Src& src, const Pds::TypeId& type, void* payload);
  public:                     // Analysis functions
    void clear    ();         // remove Entry's
    void create   (Cds& cds); // create Entry's
    void analyze  ();         // fill   Entry's
    void insert(SyncAnalysis* h)   { _syncAnalysisPList.push_back(h); }
    void remove(SyncAnalysis* h)   { _syncAnalysisPList.remove(h);    }
    void insertEntry(EntryTH1F* h) { _summaryEntryEList.push_back(h); }
    void removeEntry(EntryTH1F* h) { _summaryEntryEList.remove(h);    }
    void processAcqirisData(SyncAnalysis* syncPtr);
    void processOpalData(SyncAnalysis* syncPtr);
    void processPrincetonData(SyncAnalysis* syncPtr);
    void processGasDetectorData(SyncAnalysis* syncPtr);
    void processEBeamData(SyncAnalysis* syncPtr);
    void processPhaseCavityData(SyncAnalysis* syncPtr);
    void processPnccdData(SyncAnalysis* syncPtr);
    void processIpimbData(SyncAnalysis* syncPtr);
    void processFccdData(SyncAnalysis* syncPtr);
    void processPulnixData(SyncAnalysis* syncPtr);
    void refillSummaryPlot(SyncAnalysis* syncPtr, EntryTH1F* summaryGoodEntry, EntryTH1F* summaryDarkEntry, unsigned points, unsigned refillType) ;

  private:
    typedef std::list<Ami::SyncAnalysis*> PList;
    typedef std::list<Ami::EntryTH1F*> EList;
    PList _syncAnalysisPList;
    EList _summaryEntryEList;
	   unsigned _summaryEntries;
    unsigned _analyzeCount; 
    unsigned _notRefilledCount;
	   bool _darkShot;
    Pds::DetInfo _acqDetInfo;
    Pds::Acqiris::ConfigV1 _acqConfig;
    Pds::EvrData::DataV3* _evrEventData;
    

  };
};

#endif
