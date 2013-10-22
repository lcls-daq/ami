#ifndef TimeTool_hh
#define TimeTool_hh


#include "ami/data/UserModule.hh"  // Plug-in module interface definition

#include "pdsdata/xtc/ClockTime.hh"  // Event Timestamp definition

namespace Pds { 
  namespace Opal1k  { class ConfigV1; } 
  namespace EvrData { class DataV3; } 
  namespace Lusi    { class IpmFexV1; } 
  namespace Camera  { class FrameV1; } 
};

namespace Ami {
  class Cds;         // Plot data service
  class EntryScalar; // History plot
  class EntryTH1F;   // Distribution plot
  class EntryScan;   // Scatter plot
  class FeatureCache;
  class FexM;

  class TimeToolM : public UserModule {
  public:
    TimeToolM();
    ~TimeToolM();
  public:  // Handler functions
    void reset    (Ami::FeatureCache&);
    void clock    (const Pds::ClockTime& clk);     // timestamp of current event/configure
    void configure(const Pds::DetInfo&   src,      // configuration data callback
		   const Pds::TypeId&    type,
		   void*                 payload);
    void event    (const Pds::DetInfo&   src,      // event data callback
		   const Pds::TypeId&    type,
		   void*                 payload);
  public:  // Analysis functions
    void clear    ();         // remove Entry's
    void create   (Cds& cds); // create Entry's
    void analyze  ();         // fake
    const char* name() const;
    bool accept ();

  private:
    //  Resulting Plots
    Ami::Cds* _cds;
    EntryTH1F* _ref_signal;
    EntryTH1F* _raw_signal;
    EntryTH1F* _sub_signal;
    EntryTH1F* _sub_signal_u;
    EntryTH1F* _flt_signal;

    EntryScan* _p0corr;
    EntryScan* _p1corr;

    FexM*     _fex;
    uint32_t* _sideband;
    uint32_t* _signal;
    uint32_t* _reference;

    Pds::Camera::FrameV1* _frame;
    Pds::EvrData::DataV3* _evrdata;
    Pds::Lusi::IpmFexV1* _ipmdata;

    int _id_sig;
    int _id_sb;
    int _id_ref;
    const uint32_t* _sig_wf;
    const uint32_t* _sb_wf;
    const uint32_t* _ref_wf;

    Pds::ClockTime       _clk;
    Ami::FeatureCache*   _cache;
    int                  _cache_index;

  public:
    /// Empty instanciations
    void configure(const Pds::BldInfo&   src,      // configuration data callback
		   const Pds::TypeId&    type,
		   void*                 payload) {}
    void configure(const Pds::ProcInfo&  src,      // configuration data callback
		   const Pds::TypeId&    type,
		   void*                 payload) {}
    void event    (const Pds::BldInfo&   src,      // event data callback
		   const Pds::TypeId&    type,
		   void*                 payload) {}
    void event    (const Pds::ProcInfo&  src,      // event data callback
		   const Pds::TypeId&    type,
		   void*                 payload) {}

  };

};

#endif
