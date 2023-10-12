#ifndef BlackHole_hh
#define BlackHole_hh

#include "pdsdata/psddl/cspad.ddl.h"

#include <vector>

namespace Ami {
  class Point {
  public:
    Point(unsigned r, unsigned c) {
      _val = (r<<16)|c;
    }
    unsigned r() {return _val>>16 & 0xffff;}
    unsigned c() {return _val & 0xffff;}
  private:
    unsigned _val;
  };

  class BlackHole {
  public:
    typedef int32_t conmap_t;
    typedef int16_t data_t;
    BlackHole(unsigned nrows,  unsigned ncols, data_t thresh,
              unsigned holeNpixMin, unsigned holeNpixMax, unsigned detsMax) :
      _thresh(thresh), _holeNpixMin(holeNpixMin), _holeNpixMax(holeNpixMax),
      _detsMax(detsMax)
    {
      // the "connected map"
      _conmap = make_ndarray<conmap_t>(nrows, ncols);
      _stack.reserve(185*388*64);
      _ndump = 0;
    }
    bool floodFill(ndarray<const data_t, 3> data, uint quad, unsigned currentDet);
    const ndarray<conmap_t, 2>& conmap() {return _conmap;}
    unsigned numreg() {return _numreg;}
    unsigned lasttrip_sec() {return _lasttrip_sec;}
    unsigned lasttrip_pixcount() {return _lasttrip_pixcount;}
    void setupGoodPix(ndarray<const data_t, 3> data, uint quad, unsigned currentDet);
    void updateGoodPix(ndarray<const data_t, 3> data, uint quad, unsigned currentDet);

  private:
    static const int MAXREGION = 50;
    static const unsigned MAXDUMP = 5;
    static const unsigned MAXPRINT = 50;
    unsigned _count[MAXREGION];
    bool _edge[MAXREGION];

    bool _maybePush(unsigned sec, unsigned r, unsigned c,
                    const ndarray<const data_t, 3>& data);
    bool _trip();
    void _dump(ndarray<const data_t, 3> data);
    ndarray<conmap_t, 2> _conmap;
    unsigned _numreg;
    data_t _thresh;
    unsigned _holeNpixMin;
    unsigned _holeNpixMax;
    unsigned _detsMax;
    unsigned _lasttrip_sec;
    unsigned _lasttrip_pixcount;
    unsigned _ndump,_nprint;
    std::vector<Point> _stack;
    ndarray<data_t, 4> _goodpix;
  };
}

#endif
