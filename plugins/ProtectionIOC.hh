#ifndef ProtectionIOC_hh
#define ProtectionIOC_hh

#include <map>
#include <string>
#include <stdint.h>

namespace Pds {
  class DetInfo;
  class TypeId;
  namespace Epics { class EpicsPvHeader; }
}

namespace Ami_Epics { class PVWriter; };

namespace Ami {
  class NameService;

  class PVHandler {
  public:
    PVHandler(const std::string& pvbase);
    virtual ~PVHandler();

    void configure(const char* pvname, int16_t pvid);

    void event(const Pds::Epics::EpicsPvHeader* epics);

    void trip() const;

    void reset();

    int32_t threshold() const { return _thres_value; }
    int32_t npixels() const { return _npixel_value; }
    bool enabled() const { return _enable_value; }

  private:
    std::string _pvbase;
    std::string _thres_pv;
    std::string _npixel_pv;
    std::string _enable_pv;
    std::string _shutter_pv;
    int16_t _thres_epics;
    int16_t _npixel_epics;
    int16_t _enable_epics;
    int32_t _thres_value;
    int32_t _npixel_value;
    bool    _enable_value;
    Ami_Epics::PVWriter* _shutter;
  };

  class Threshold {
  public:
    Threshold(const char* fname, NameService* name_service);
    virtual ~Threshold();

    void configure(const Pds::DetInfo&  src,
                   const Pds::TypeId&   type,
                   void*                payload);

    void event(const Pds::Epics::EpicsPvHeader* epics);

    void reset();

    const PVHandler* lookup(const Pds::DetInfo&  src,
                            const Pds::TypeId&   type) const;

  private:
    const char* _fname;
    NameService* _name_service;
    std::map<Pds::DetInfo, PVHandler*> _detpvs;
    std::map<std::string, PVHandler*>  _typepvs;
  };
}

#endif
