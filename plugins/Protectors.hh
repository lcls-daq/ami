#ifndef Protectors_hh
#define Protectors_hh

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include "ndarray/ndarray.h"

#include <string>
#include <stdint.h>

namespace Pds {
  namespace CsPad {
    class ConfigV5;
    class DataV2;
  }
}

namespace Ami {
  class PVHandler;
  class Threshold;
  class BlackHole;
  class EntryScan;
  class EntryScalar;
  class FeatureCache;

  class Protector {
  public:
    Protector(const char* pname,
              const Pds::DetInfo& info,
              const PVHandler* handler,
              bool uses_bh=false);
    virtual ~Protector();
    static Protector* instance(const Pds::DetInfo& info,
                               const Pds::TypeId& type,
                               void* payload,
                               const Threshold* threshold);

    const Pds::DetInfo& info() const;
    void setName(const char* name);
    void clear();
    bool hasEntry() const;
    bool hasScan() const;
    EntryScalar* entry();
    EntryScan* scan();

    void cache(FeatureCache* cache);

    void accept(const Pds::ClockTime& clk);

    virtual void event(const Pds::TypeId& type,
                       void* payload) = 0;

  protected:
    virtual bool ready() const = 0;
    virtual bool analyzeDetector(const Pds::ClockTime& clk, int32_t& pixelCount) = 0;

  protected:
    const char*        _pname;
    std::string        _name;
    const Pds::DetInfo _info;
    EntryScalar*       _entry;
    EntryScan*         _scan;
    const PVHandler*   _handler;
    bool               _uses_bh;
    unsigned           _nevt;
    Pds::ClockTime     _lastTrip;
    FeatureCache*      _cache;
    int                _npoints_index;
    int                _tripped_index;
    int                _evttime_index;
  };

  class CsPadProtector : public Protector {
  public:
    CsPadProtector(const Pds::DetInfo& info,
                   const Pds::TypeId&  type,
                   void*               payload,
                   const PVHandler*    handler);
    virtual ~CsPadProtector();

    virtual void event(const Pds::TypeId& type,
                       void* payload);
    
  protected:
    virtual bool analyzeDetector(const Pds::ClockTime& clk, int32_t& pixelCount);
    virtual bool ready() const;

  private:
    BlackHole*                _bh;
    Pds::CsPad::ConfigV5*     _config;
    uint32_t                  _config_size;
    const Pds::CsPad::DataV2* _data;
  };

  template<class Cfg, class Data>
  class JungfrauProtector : public Protector {
  public:
    JungfrauProtector(const Pds::DetInfo& info,
                      const Pds::TypeId&  type,
                      void*               payload,
                      const PVHandler*    handler);
    virtual ~JungfrauProtector();

    virtual void event(const Pds::TypeId& type,
                       void* payload);

  protected:
    virtual bool analyzeDetector(const Pds::ClockTime& clk, int32_t& pixelCount);
    virtual bool ready() const;

  private:
    Cfg*        _config;
    uint32_t    _config_size;
    const Data* _data;
  };

  template<class Cfg, class Data>
  class EpixArrayProtector : public Protector {
  public:
    EpixArrayProtector(const Pds::DetInfo& info,
                       const Pds::TypeId&  type,
                       void*               payload,
                       const PVHandler*    handler);
    virtual ~EpixArrayProtector();

    virtual void event(const Pds::TypeId& type,
                       void* payload);

  protected:
    enum { AML=0, AML_FORCE=4, FL=8, FM=12, AHL=16, AHL_FORCE=20, FL_ALT=24, FH=28 };
    static const unsigned conf_bits = 0x1c;
    static const unsigned nE;
    static const unsigned wE;
    static const unsigned hE;
    static const unsigned asicMap[4];

    virtual bool analyzeDetector(const Pds::ClockTime& clk, int32_t& pixelCount);
    virtual bool ready() const;

  private:
    Cfg*                _config;
    uint32_t            _config_size;
    const Data*         _data;
    unsigned            _gain_mask;
    ndarray<uint16_t,3> _gain_cfg;
  };

  template<class Cfg, class Data>
  class Epix10kaProtector : public Protector {
  public:
    Epix10kaProtector(const Pds::DetInfo& info,
                      const Pds::TypeId&  type,
                      void*               payload,
                      const PVHandler*    handler);
    virtual ~Epix10kaProtector();

    virtual void event(const Pds::TypeId& type,
                       void* payload);

    protected:
      enum { AML=0, AML_FORCE=4, FL=8, FM=12, AHL=16, AHL_FORCE=20, FL_ALT=24, FH=28 };
      static const unsigned conf_bits = 0x1c;
      static const unsigned asicMap[4];

      virtual bool analyzeDetector(const Pds::ClockTime& clk, int32_t& pixelCount);
      virtual bool ready() const;

    private:
      Cfg*                _config;
      uint32_t            _config_size;
      const Data*         _data;
      unsigned            _gain_mask;
      ndarray<uint16_t,2> _gain_cfg;
  };

  template<class Cfg, class Data>
  class EpixProtector : public Protector {
  public:
    EpixProtector(const Pds::DetInfo& info,
                  const Pds::TypeId&  type,
                  void*               payload,
                  const PVHandler*    handler);
    virtual ~EpixProtector();

    virtual void event(const Pds::TypeId& type,
                       void* payload);

  protected:
    virtual bool analyzeDetector(const Pds::ClockTime& clk, int32_t& pixelCount);
    virtual bool ready() const;

  private:
    Cfg*        _config;
    uint32_t    _config_size;
    const Data* _data;
  };
}

#endif
