#ifndef AmiQt_Rect_hh
#define AmiQt_Rect_hh

namespace Ami {
  namespace Qt {
    class Rect {
    public:
      unsigned x0,y0;
      unsigned x1,y1;
    public:
      void save(char*&) const;
      void load(const char*&);
    };
  };
};

#endif
