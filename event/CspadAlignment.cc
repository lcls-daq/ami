#include "ami/event/CspadAlignment.hh"
#include "ami/event/Calib.hh"

#include <string>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace Ami;
using Ami::Cspad::QuadAlignment;
using Ami::Cspad::TwoByOneAlignment;

static const double asicWidth  = 110*194; // 20.335e-3 m
static const double asicHeight = 110*185;

static const Ami::Cspad::QuadAlignment qproto = {
  {  { {   {21757, 33110}, {21757, 33110}, {21757, 33110}, {21757, 33110} }, {21757, 33110}, (Ami::Rotation)3 },
     { {   {21769, 10457}, {21769, 10457}, {21769, 10457}, {21769, 10457} }, {21769, 10457}, (Ami::Rotation)3 },
     { {   {33464, 68275}, {33464, 68275}, {33464, 68275}, {33464, 68275} }, {33464, 68275}, (Ami::Rotation)2 },
     { {   {10769, 68299}, {10769, 68299}, {10769, 68299}, {10769, 68299} }, {10769, 68299}, (Ami::Rotation)2 },
     { {   {68489, 56732}, {68489, 56732}, {68489, 56732}, {68489, 56732} }, {68489, 56732}, (Ami::Rotation)1 },
     { {   {68561, 79628}, {68561, 79628}, {68561, 79628}, {68561, 79628} }, {68561, 79628}, (Ami::Rotation)1 },
     { {   {77637, 21754}, {77637, 21754}, {77637, 21754}, {77637, 21754} }, {77637, 21754}, (Ami::Rotation)0 },
     { {   {54810, 21558}, {54810, 21558}, {54810, 21558}, {54810, 21558} }, {54810, 21558}, (Ami::Rotation)0 } }
};


//
//  Apply a clockwise rotation (in a RHS)
//
static void _transform(double& x,double& y,double dx,double dy,Rotation r)
{
  switch(r) {
  case D0  :    x += dx; y += dy; break;
  case D90 :    x -= dy; y += dx; break;
  case D180:    x -= dx; y -= dy; break;
  case D270:    x += dy; y -= dx; break;
  default:                        break;
  }
}

static void _sort_and_store(Ami::Cspad::QuadAlignment* nq,
                            Ami::Cspad::QuadAlignment* q);

//
//  Read alignment from corner locations and beam position
//  in a Right Handed System.
//  Rotate coordinates to be in x>0, y>0 quadrant
//
Ami::Cspad::QuadAlignment* QuadAlignment::load(FILE* g, bool offline)
{
  Ami::Cspad::QuadAlignment q[4];
  Ami::Cspad::QuadAlignment* nq = new Ami::Cspad::QuadAlignment[4];

  size_t sz=256;
  char* linep = (char *)malloc(sz);

  if (1) {
  //  if (offline) {  // Read offline style file { RHS + relative coordinates + rotations }

    while(1) {
      if (Ami::Calib::get_line(&linep, &sz, g)<0) break;
      if (strlen(linep)==0) continue;
      if (linep[0]=='#') continue;

      std::string pname;
      unsigned    pindex;
      std::string oname;
      unsigned    oindex;
      double      x0;
      double      y0;
      double      z0;
      double      rot_z;
      double      rot_y;
      double      rot_x;                  
      double      tilt_z;
      double      tilt_y;
      double      tilt_x; 
      
      std::string sline(linep);
      std::stringstream ss(sline);

      ss >> pname >> pindex >> oname >> oindex >> x0 >> y0 >> z0 
         >> rot_z >> rot_y >> rot_x >> tilt_z >> tilt_y >> tilt_x;

      if (pname.size()) {
        if (oname == "SENS2X1:V1") {
          for(unsigned c=0; c<4; c++) {
            q[pindex]._twobyone[oindex]._corner[c].x = x0;
            q[pindex]._twobyone[oindex]._corner[c].y = y0;
          }
          q[pindex]._twobyone[oindex]._pad.x = x0;
          q[pindex]._twobyone[oindex]._pad.y = y0;
          q[pindex]._twobyone[oindex]._rot   = Rotation(unsigned(rot_z/90+3.5)%NPHI);
        }
        else if (oname == "QUAD:V1" ||
                 oname == "QUAD:V2") {
          for(unsigned k=0; k<8; k++) {
            Rotation irot = Rotation(unsigned(rot_z/90.+0.5)%NPHI);
            double dx=x0, dy=y0;
            struct TwoByOneAlignment& twobyone = q[oindex]._twobyone[k];
            _transform(dx,dy,
                       twobyone._pad.x,
                       twobyone._pad.y,
                       irot);
            for(unsigned c=0; c<4; c++) {
              twobyone._corner[c].x = dx;
              twobyone._corner[c].y = dy;
            }
            twobyone._pad.x = dx;
            twobyone._pad.y = dy;
            twobyone._rot   = Rotation(unsigned(irot+twobyone._rot)%NPHI);
          }
        }
      }
    }

    _sort_and_store(nq, q);
  }

  if (linep) {
    free(linep);
  }
  return nq;
}

//
//  Read alignment from corner locations and beam position
//  in a Right Handed System.
//  Rotate coordinates to be in x>0, y>0 quadrant
//
Ami::Cspad::QuadAlignment* QuadAlignment::load2x2(FILE* g, bool offline)
{
  Ami::Cspad::QuadAlignment q;
  Ami::Cspad::QuadAlignment* nq = new Ami::Cspad::QuadAlignment;

  size_t sz=256;
  char* linep = (char *)malloc(sz);

  bool lerror=false;

  if (1) {
    //  if (offline) {  // Read offline style file { RHS + relative coordinates + rotations }

    while(1) {
      if (Ami::Calib::get_line(&linep, &sz, g)<0) break;
      if (strlen(linep)==0) continue;
      if (linep[0]=='#') continue;

      std::string pname;
      unsigned    pindex;
      std::string oname;
      unsigned    oindex;
      double      x0;
      double      y0;
      double      z0;
      double      rot_z;
      double      rot_y;
      double      rot_x;                  
      double      tilt_z;
      double      tilt_y;
      double      tilt_x; 
      
      std::string sline(linep);
      std::stringstream ss(sline);

      ss >> pname >> pindex >> oname >> oindex >> x0 >> y0 >> z0 
         >> rot_z >> rot_y >> rot_x >> tilt_z >> tilt_y >> tilt_x;

      if (pname.size()) {
        if (oname == "SENS2X1:V1") {
          if (oindex<2 && pindex==0) {
            for(unsigned c=0; c<4; c++) {
              q._twobyone[oindex]._corner[c].x = x0;
              q._twobyone[oindex]._corner[c].y = y0;
            }
            q._twobyone[oindex]._pad.x = x0;
            q._twobyone[oindex]._pad.y = y0;
            q._twobyone[oindex]._rot   = Rotation(unsigned(rot_z/90+3.5)%NPHI);
          }
          else {
            printf("Unexpected index in line [%s]\n",linep);
            lerror = true;
            break;
          }
        }
        else if (oname == "CSPAD2X2:V1") {
          if (oindex==0) {
            for(unsigned k=0; k<2; k++) {
              Rotation irot = Rotation(unsigned(rot_z/90.+0.5)%NPHI);
              double dx=x0, dy=y0;
              struct TwoByOneAlignment& twobyone = q._twobyone[k];
              _transform(dx,dy,
                         twobyone._pad.x,
                         twobyone._pad.y,
                         irot);
              for(unsigned c=0; c<4; c++) {
                twobyone._corner[c].x = dx;
                twobyone._corner[c].y = dy;
              }
              twobyone._pad.x = dx;
              twobyone._pad.y = dy;
              twobyone._rot   = Rotation(unsigned(irot+twobyone._rot)%NPHI);
            }
          }
          else {
            printf("Unexpected index in line [%s]\n",linep);
            lerror = true;
            break;
          }
        }
      }
    }

    // _sort_and_store(nq, q);
    if (lerror) {
      *nq = qproto;
    }
    else {
      double bx=0.5*(q._twobyone[0]._pad.x+q._twobyone[1]._pad.x);
      double by=0.5*(q._twobyone[0]._pad.y+q._twobyone[1]._pad.y);

      for(unsigned k=0; k<2; k++) {
        struct TwoByOneAlignment& ntwobyone = nq->_twobyone[k];
        struct TwoByOneAlignment& otwobyone = q  ._twobyone[k];

        for(unsigned c=0; c<4; c++) {
          ntwobyone._corner[c].x = otwobyone._corner[c].x-bx;
          ntwobyone._corner[c].y = otwobyone._corner[c].y-by;
          _transform(ntwobyone._corner[c].x,
                     ntwobyone._corner[c].y,
                     -0.5*asicHeight,
                     -asicWidth,
                     otwobyone._rot);
        }
        ntwobyone._pad.x = ntwobyone._corner[0].x;
        ntwobyone._pad.y = ntwobyone._corner[0].y;
        ntwobyone._rot   = Rotation((4-otwobyone._rot)%NPHI);
      }
    }
  }

  if (linep) {
    free(linep);
  }
  return nq;
}

//
//  Index the sections correctly (by readout order)
//    and move section centers to section origin (first readout pixel)
//
void _sort_and_store(Ami::Cspad::QuadAlignment* nq,
                     Ami::Cspad::QuadAlignment* q)
{
  double bx,by;

  bx = 0;
  by = 0;
  for(unsigned iq=0; iq<4; iq++) {
    for(unsigned i=0; i<8; i++) { // loop over 2x1's
      bx += q[iq]._twobyone[i]._pad.x;
      by += q[iq]._twobyone[i]._pad.y;
    }
  }
  bx /= 32.;
  by /= 32.;

  for(unsigned iq=0; iq<4; iq++) {
    double qx=0, qy=0;
    for(unsigned i=0; i<8; i++) { // loop over 2x1's
      qx += 0.125*q[iq]._twobyone[i]._pad.x;
      qy += 0.125*q[iq]._twobyone[i]._pad.y;
    }

    Rotation qr;
    if (qx < bx) { qr = (qy<by) ? D180 : D90; }
    else         { qr = (qy<by) ? D270 : D0;  }

    /*
    **  Index the sections accordingly.
    **
    **    +---+ +---+ +---------+
    **    |   | |   | |    4    |
    **    | 2 | | 3 | +---------+
    **    |   | |   | +---------+
    **    |   | |   | |    5    |
    **    +---+ +---+ +---------+
    **    +---------+ +---+ +---+
    **    |    1    | |   | |   |
    **    +---------+ | 7 | | 6 |
    **    +---------+ |   | |   |
    **    |    0    | |   | |   |
    **    +---------+ +---+ +---+
    */
    unsigned si[8];
    for(unsigned i=0; i<8; i++) {
      if (q[iq]._twobyone[i]._pad.x < qx) {
        if (q[iq]._twobyone[i]._pad.y + asicHeight < qy)
          si[i]=0;
        else if (q[iq]._twobyone[i]._pad.y < qy)
          si[i]=1;
        else if (q[iq]._twobyone[i]._pad.x + asicHeight < qx) 
          si[i]=2;
        else
          si[i]=3;
      }
      else {
        if (q[iq]._twobyone[i]._pad.y - asicHeight > qy)
          si[i]=4;
        else if (q[iq]._twobyone[i]._pad.y > qy)
          si[i]=5;
        else if (q[iq]._twobyone[i]._pad.x - asicHeight > qx) 
          si[i]=6;
        else
          si[i]=7;
      }
    }
    
    static const unsigned index[] = { 1,0,3,2,5,4,6,7,
                                      3,2,5,4,6,7,1,0,
                                      5,4,6,7,1,0,3,2,
                                      6,7,1,0,3,2,5,4 };
    //  Center the structure and
    //  place the location of the first readout pixel
    for(unsigned i=0; i<8; i++) {
      unsigned k = index[qr*8+si[i]];
      struct TwoByOneAlignment& ntwobyone = nq[iq]._twobyone[k];
      struct TwoByOneAlignment& otwobyone = q [iq]._twobyone[i];
      for(unsigned c=0; c<4; c++) {
        ntwobyone._corner[c].x = otwobyone._corner[c].x-bx;
        ntwobyone._corner[c].y = otwobyone._corner[c].y-by;
        _transform(ntwobyone._corner[c].x,
                   ntwobyone._corner[c].y,
                   -0.5*asicHeight,
                   -asicWidth,
                   otwobyone._rot);
      }
      ntwobyone._pad.x = ntwobyone._corner[0].x;
      ntwobyone._pad.y = ntwobyone._corner[0].y;
      ntwobyone._rot   = Rotation((4-otwobyone._rot)%NPHI);
    }
  }
}

static Ami::Cspad::QuadAlignment* _instance = 0;

const Ami::Cspad::QuadAlignment* Ami::Cspad::QuadAlignment::qalign_def() 
{
  if (_instance)
    return _instance;

  Ami::Cspad::QuadAlignment q[4];
  static const double _dx[] = { -4500, -3500, 5500, 4500 };
  static const double _dy[] = { -2700, 5800, 4500, -4500 };
  for(unsigned j=0; j<4; j++) {
    q[j] = qproto;

    Rotation irot = Rotation(unsigned(5-j)%NPHI);

    for(unsigned k=0; k<8; k++) {
      double dx = _dx[j];
      double dy = _dy[j];
      struct TwoByOneAlignment& twobyone = q[j]._twobyone[k];
      _transform(dx,dy,
                 twobyone._pad.x,
                 twobyone._pad.y,
                 irot);
      for(unsigned c=0; c<4; c++) {
        twobyone._corner[c].x = dx;
        twobyone._corner[c].y = dy;
      }
      twobyone._pad.x = dx;
      twobyone._pad.y = dy;
      twobyone._rot   = Rotation(unsigned(irot+twobyone._rot)%NPHI);
    }
  }
  _instance = new Ami::Cspad::QuadAlignment[4];
  _sort_and_store(_instance, q);
  return _instance;
}
