#ifndef SummaryAnalysis_hh
#define SummaryAnalysis_hh

namespace Pds {
  class ClockTime;
  class Src;
  class TypeId;
};

namespace Ami {
  class Cds;

  class SummaryAnalysis {
  public:
    static SummaryAnalysis& instance();
  private:
    SummaryAnalysis();
    ~SummaryAnalysis();
  public:  // Handler functions
    void reset    ();
    void clock    (const Pds::ClockTime& clk);
    void configure(const Pds::Src&       src,
		   const Pds::TypeId&    type,
		   void*                 payload);
    void event    (const Pds::Src&       src,
		   const Pds::TypeId&    type,
		   void*                 payload);
  public:  // Analysis functions
    void clear    ();         // remove Entry's
    void create   (Cds& cds); // create Entry's
    void analyze  ();         // fill   Entry's
  };
};

#endif
