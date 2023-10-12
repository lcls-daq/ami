#ifndef Protectors_hh
#define Protectors_hh

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/ClockTime.hh"

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
  class EntryScalar;

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
    EntryScalar* entry();

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
    const PVHandler*   _handler;
    bool               _uses_bh;
    unsigned           _nevt;
    Pds::ClockTime     _lastTrip;
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
    virtual bool analyzeDetector(const Pds::ClockTime& clk, int32_t& pixelCount);
    virtual bool ready() const;

  private:
    Cfg*        _config;
    uint32_t    _config_size;
    const Data* _data;
  };
}

#endif
