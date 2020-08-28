#include "ami/event/EpixAlignment.hh"

#include "pdsdata/psddl/epix.ddl.h"

using namespace Ami::Alignment;

typedef Pds::Epix::Config10ka2MV1   Cfg10ka2M;
typedef Pds::Epix::Config10kaQuadV1 Cfg10kaQuad;
typedef Pds::Epix::Config10ka       Cfg10ka;

static const unsigned      wE = Cfg10ka::_numberOfPixelsPerAsicRow*Cfg10ka::_numberOfAsicsPerRow;
static const unsigned      hE = Cfg10ka::_numberOfRowsPerAsic     *Cfg10ka::_numberOfAsicsPerColumn;
//  Three margin parameters
static const unsigned      gM = 2;
static const unsigned      eM = 80;  // "edge" margin
static const unsigned      hM = 4;   // 1/2 "horizontal" margin between ASICs
static const unsigned      vM = 12;  // 1/2 "vertical" margin between ASICs
static const double pixel_size = 100.0; // um

Epix10ka2M::Epix10ka2M(const Pds::DetInfo& det, unsigned index) :
  Detector(det, "CAMERA", "QUAD", "EPIX10KA:V1",
           4, Cfg10ka2M::_numberOfElements, pixel_size,
           wE, hE,
           index)
{
  if (_use_default) load_default();
}

Epix10ka2M::~Epix10ka2M()
{}

void Epix10ka2M::load_default()
{
  //
  //   Hard code the array's geometry for now
  //
  //  (Epix10ka2m)
  //         |
  //  Quad 0 | Quad 1      Quad 2 is rotated  90d clockwise
  //  -------+--------     Quad 3 is rotated 180d clockwise
  //  Quad 3 | Quad 2      Quad 0 is rotated 270d clockwise
  //         |
  //
  //  (Quad 1)
  //         |
  //  Elem 0 | Elem 1
  //  -------+--------     No rotations
  //  Elem 2 | Elem 3
  //         |
  //
  //  (Elem 0)
  //         |
  //  ASIC 0 | ASIC 3
  //  -------+--------     No rotations
  //  ASIC 1 | ASIC 2
  //         |
  //
  //  (Elem 0-3 pixel array)
  //                    row increasing
  //                          ^
  //                          |
  //                          |
  //  column increasing <-- (0,0)
  //
  //                                         eM     vM     hM     hE     wE, eM     vM     hM     hE       wE
  static const SubFrame elem[] = { SubFrame( eM + 1*vM                     ,             3*hM         + 1*wE, hE, wE, D270 ),
                                   SubFrame( eM + 1*vM                     ,             1*hM               , hE, wE, D270 ),
                                   SubFrame( eM + 3*vM        + 1*hE       ,             3*hM         + 1*wE, hE, wE, D270 ),
                                   SubFrame( eM + 3*vM        + 1*hE       ,             1*hM               , hE, wE, D270 ),

                                   SubFrame( eM + 4*vM + 1*hM + 2*hE       , eM + 1*vM                      , wE, hE, D180 ),
                                   SubFrame( eM + 4*vM + 3*hM + 2*hE + 1*wE, eM + 1*vM                      , wE, hE, D180 ),
                                   SubFrame( eM + 4*vM + 1*hM + 2*hE       , eM + 3*vM        + 1*hE        , wE, hE, D180 ),
                                   SubFrame( eM + 4*vM + 3*hM + 2*hE + 1*wE, eM + 3*vM        + 1*hE        , wE, hE, D180 ),

                                   SubFrame(      3*vM + 3*hM + 1*hE + 2*wE, eM + 3*vM + 2*hM + 2*hE        , hE, wE, D90 ),
                                   SubFrame(      3*vM + 3*hM + 1*hE + 2*wE, eM + 3*vM + 4*hM + 2*hE  + 1*wE, hE, wE, D90 ),
                                   SubFrame(      1*vM + 3*hM        + 2*wE, eM + 3*vM + 2*hM + 2*hE        , hE, wE, D90 ),
                                   SubFrame(      1*vM + 3*hM        + 2*wE, eM + 3*vM + 4*hM + 2*hE  + 1*wE, hE, wE, D90 ),

                                   SubFrame(             3*hM        + 1*wE,      3*vM + 3*hM + 1*hE  + 2*wE, wE, hE, D0 ),
                                   SubFrame(             1*hM              ,      3*vM + 3*hM + 1*hE  + 2*wE, wE, hE, D0 ),
                                   SubFrame(             3*hM        + 1*wE,      1*vM + 3*hM         + 2*wE, wE, hE, D0 ),
                                   SubFrame(             1*hM              ,      1*vM + 3*hM         + 2*wE, wE, hE, D0 ) };

  unsigned xmin=-1U, xmax=0, ymin=-1U, ymax=0, v;
  for(unsigned i=0; i<_elements.size(); i++) {
    // cleanup any elements that were created
    if (_elements[i]) {
      delete _elements[i];
      _elements[i] = NULL;
    }

    // calculate overall size
    if ((v=elem[i].x             ) < xmin) xmin = v;
    if ((v=elem[i].x + elem[i].nx) > xmax) xmax = v;
    if ((v=elem[i].y)              < ymin) ymin = v;
    if ((v=elem[i].y + elem[i].ny) > ymax) ymax = v;
  }

  // set overall dimensions
  _width  = xmax - xmin + 2*gM;
  _height = ymax - ymin + 2*gM;

  // set overall rotation to zero
  _rotation = D0;

  for(unsigned i=0; i<_elements.size(); i++) {
    // cleanup any elements that were created
    if (_elements[i]) {
      delete _elements[i];
      _elements[i] = NULL;
    }

    // shift the frame coordinates
    SubFrame fr(elem[i]);
    fr.x += gM - xmin;
    fr.y += gM - ymin;
    // create a new element
    _elements[i] = new Element(fr);
  }
}

Epix10kaQuad::Epix10kaQuad(const Pds::DetInfo& det, unsigned index) :
  Detector(det, "CAMERA", "QUAD", "EPIX10KA:V1",
           1, Cfg10kaQuad::_numberOfElements, pixel_size,
           wE, hE,
           index)
{
  if (_use_default) load_default();
}

Epix10kaQuad::~Epix10kaQuad()
{}

void Epix10kaQuad::load_default()
{
  //
  //   Hard code the array's geometry for now
  //
  //  (Epix10kaQuad)
  //         |
  //  Elem 0 | Elem 1
  //  -------+--------     No rotations
  //  Elem 2 | Elem 3
  //         |
  //
  //  (Elem 0)
  //         |
  //  ASIC 0 | ASIC 3
  //  -------+--------     No rotations
  //  ASIC 1 | ASIC 2
  //         |
  static const SubFrame elem[] = { SubFrame( eM + 4*vM + 1*hM + 2*hE       , eM + 1*vM                      , wE, hE, D180 ),
                                   SubFrame( eM + 4*vM + 3*hM + 2*hE + 1*wE, eM + 1*vM                      , wE, hE, D180 ),
                                   SubFrame( eM + 4*vM + 1*hM + 2*hE       , eM + 3*vM        + 1*hE        , wE, hE, D180 ),
                                   SubFrame( eM + 4*vM + 3*hM + 2*hE + 1*wE, eM + 3*vM        + 1*hE        , wE, hE, D180 ) };

  unsigned xmin=-1U, xmax=0, ymin=-1U, ymax=0, v;
  for(unsigned i=0; i<_elements.size(); i++) {
    // cleanup any elements that were created
    if (_elements[i]) {
      delete _elements[i];
      _elements[i] = NULL;
    }

    // calculate overall size
    if ((v=elem[i].x             ) < xmin) xmin = v;
    if ((v=elem[i].x + elem[i].nx) > xmax) xmax = v;
    if ((v=elem[i].y)              < ymin) ymin = v;
    if ((v=elem[i].y + elem[i].ny) > ymax) ymax = v;
  }

  // set overall dimensions
  _width  = xmax - xmin + 2*gM;
  _height = ymax - ymin + 2*gM;

  // set overall rotation to zero
  _rotation = D0;

  for(unsigned i=0; i<_elements.size(); i++) {
    // cleanup any elements that were created
    if (_elements[i]) {
      delete _elements[i];
      _elements[i] = NULL;
    }

    // shift the frame coordinates
    SubFrame fr(elem[i]);
    fr.x += gM - xmin;
    fr.y += gM - ymin;
    // create a new element
    _elements[i] = new Element(fr);
  }
}
