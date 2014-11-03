#ifndef Pds_DESCENTRY_HH
#define Pds_DESCENTRY_HH

#include "ami/data/Desc.hh"

#include "pdsdata/xtc/DetInfo.hh"

namespace Ami {

  /**
   *   A class for describing the shape and meaningful content of transmitted data.
   *   This includes number of dimensions, dimension names, dimension sizes, 
   *   integer/float representation, binning, detector source, and some options for
   *   treatment of the data.  
   *
   *   The contents of this class are transmitted directly over the network, so no 
   *   virtual tables are allowed.  In addition, derived classes are enumerated and of
   *   fixed size.
   */
  class DescEntry : public Desc {
  public:
    ///  Enumeration of subclasses
    enum Type {Scalar, TH1F, TH2F, Prof, Image, Waveform, Scan, Ref, Cache, ScalarRange, ScalarDRange, Prof2D };
    Type type() const;
    static const char* type_str(Type);

    ///  Returns an object identifying the source of the data it describes
    const Pds::DetInfo& info()    const;
    unsigned            channel() const;

    ///  Returns a title for the x-dimension data
    const char* xtitle() const;
    ///  Returns a title for the y-dimension data
    const char* ytitle() const;
    ///  Returns a description of the z-dimension data scaling
    const char* zunits() const;

    ///  Returns the size in bytes of this object
    unsigned short size() const;

    ///  Indicates whether the data content is to be scaled by a normalization factor
    bool isnormalized() const;
    ///  Indicates whether the data content is to be merged with data from other events
    bool aggregate   () const;
    ///  Indicates whether the data content is to be periodically refreshed
    bool check_refresh() const;
    ///  Indicates whether the data content is to be refreshed now
    bool force_refresh() const;
    ///  Indicates whether the data content is to be refreshed every update
    bool auto_refresh() const;
    ///  Indicates whether the data represents counts/occurrences or summed analog data
    bool countmode   () const;
    ///  Indicates whether the data is to be weighted by an event-variable
    bool isweighted_type() const;
    ///
    bool hasPedCalib () const;
    ///  Indicates whether the z-data has units (::zunits())
    bool hasGainCalib() const;
    ///  Indicates whether the z-data has known variances
    bool hasRmsCalib () const;
    /*
     *   Indicates whether this data is viewed or used as input to viewed data.
     *   If not, it won't be updated/processed.
     */
    bool used        () const;
    ///  Indicates whether this data source is in the recorded data stream
    bool recorded    () const;
    ///  Bit mask of previous set of options
    unsigned options() const { return _options>>User; }

    /// divide contents by normalization when displaying or reducing
    void normalize(bool);  

    /// merge distributed data = sum contents and normalization, else just keep latest set
    void aggregate(bool);  
    /// periodically refresh
    void check_refresh(bool);  
    /// refresh now
    void force_refresh(bool);  
    /// refresh every update
    void auto_refresh(bool);  
    /// bin contents count events, else sum ADUs
    void countmode(bool); 
    ///
    void pedcalib (bool);
    ///  Use zunits()
    void gaincalib(bool);
    ///  Indicate whether the z-data has known variances
    void rmscalib (bool);
    ///  Indicate whether the data needs to be updated.
    void used     (bool) const;
    ///  Indicate whether the data is in the recorded stream as a warning to the user
    void recorded (bool);
    ///  Modifier for all options
    void options  (unsigned);

  protected:
    /**
     *   Constructor without a detector data source
     *
     *   @param[in]  name          Title for the plot data
     *   @param[in]  xtitle        Title for the x-dimension data
     *   @param[in]  ytitle        Title for the y-dimension data
     *   @param[in]  type          Subclass enumeration
     *   @param[in]  size          Total size of derived object
     *   @param[in]  isnormalized  Boolean indicating to scale data by a normalization factor
     *   @param[in]  aggregate     Boolean indicating to merge data across events
     *   @param[in]  options       Set mask for remaining options
     */
    DescEntry(const char* name, const char* xtitle, const char* ytitle, 
	      Type type, unsigned short size, bool isnormalized=true, bool aggregate=true,
              unsigned options=0);

    /**
     *   Constructor from a 1-D detector data source
     *
     *   @param[in]  info          Object describing data source detector
     *   @param[in]  channel       Specific channel within that detector
     *   @param[in]  name          Title for the plot data
     *   @param[in]  xtitle        Title for the x-dimension data
     *   @param[in]  ytitle        Title for the y-dimension data
     *   @param[in]  type          Subclass enumeration
     *   @param[in]  size          Total size of derived object
     *   @param[in]  isnormalized  Boolean indicating to scale data by a normalization factor
     *   @param[in]  aggregate     Boolean indicating to merge data across events
     *   @param[in]  options       Set mask for remaining options
     */
    DescEntry(const Pds::DetInfo& info, unsigned channel,
	      const char* name, const char* xtitle, const char* ytitle, 
	      Type type, unsigned short size, bool isnormalized=true, bool aggregate=false,
              unsigned options=0);

    /**
     *   Constructor from a 2-D detector data source
     *
     *   @param[in]  info          Object describing data source detector
     *   @param[in]  channel       Specific channel within that detector
     *   @param[in]  name          Title for the plot data
     *   @param[in]  xtitle        Title for the x-dimension data
     *   @param[in]  ytitle        Title for the y-dimension data
     *   @param[in]  zunits        Title describing z-dimension data scale
     *   @param[in]  type          Subclass enumeration
     *   @param[in]  size          Total size of derived object
     *   @param[in]  isnormalized  Boolean indicating to scale data by a normalization factor
     *   @param[in]  aggregate     Boolean indicating to merge data across events
     *   @param[in]  hasPedCalib   Boolean indicating calibrated pedestal
     *   @param[in]  hasGainCalib  Boolean indicating calibrated gain (valid zunits())
     *   @param[in]  hasRmsCalib   Boolean indicating calibrated resolution
     *   @param[in]  options       Set mask for remaining options
     */
    DescEntry(const Pds::DetInfo& info, unsigned channel,
	      const char* name, const char* xtitle, const char* ytitle, const char* zunits,
	      Type type, unsigned short size, bool isnormalized=true, bool aggregate=false,
              bool hasPedCalib=false, bool hasGainCalib=false, bool hasRmsCalib=false,
              unsigned options=0);

    DescEntry(const DescEntry&);

  private:
    ///  Enumeration of options in a bit mask
    enum Option {Normalized, Aggregate, 
                 CheckRefresh, ForceRefresh, 
                 AutoRefresh,
                 CountMode, 
                 CalibMom0, CalibMom1, CalibMom2,  // has calibrations of these orders { Ped, Gain, Sigma }
                 Used, NotRecorded, User};
    void _set_opt(bool,Option);

  private:
    Pds::DetInfo _info;
    uint32_t     _channel;
    uint32_t     _reserved;
    enum {TitleSize=64};
    char _xtitle[TitleSize];
    char _ytitle[TitleSize];
    char _zunits[TitleSize];
    mutable uint32_t   _options;
    uint16_t  _type;
    uint16_t  _size;
  };
};

#endif
