#ifndef AmiQt_AxisInfo_hh
#define AmiQt_AxisInfo_hh

namespace Ami {
  namespace Qt {
    class AxisInfo {
    public:
      virtual ~AxisInfo() {}
    public:
      virtual int    lo      () const = 0;
      virtual int    hi      () const = 0;
      virtual int    center  () const = 0;
      //  Canvas pixel from detector pixel
      virtual int    tick    (double pos ) const = 0;
      virtual int    tick_u  (double pos ) const;
      //  Detector pixel from canvas pixel
      virtual double position(int    tick) const = 0;
      //  Frame pixel from detector pixel
      virtual int    ftick   (double pos ) const;
      virtual int    ftick_u (double pos ) const;
    };
  };
};

#endif
