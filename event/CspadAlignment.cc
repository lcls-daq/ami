#include "ami/event/CspadAlignment.hh"

#include <string>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define USE_PAD

using namespace Ami::Cspad;

enum Rotation { D0, D90, D180, D270, NPHI=4 };

//
//  Apply a clockwise rotation
//
static void _transform(double& x,double& y,double dx,double dy,Rotation r)
{
  switch(r) {
  case D0  :    x += dx; y += dy; break;
  case D90 :    x += dy; y -= dx; break;
  case D180:    x -= dx; y -= dy; break;
  case D270:    x -= dy; y += dx; break;
  default:                        break;
  }
}

TwoByTwoAlignment QuadAlignment::twobytwo(unsigned i) const
{
  static const Rotation r[] = { D270, D180, D90, D180 };

  TwoByTwoAlignment a;
  a.xOrigin = 0;
  a.yOrigin = 0;

  int pOrigin = 2*i+1;
#ifdef USE_PAD
  double px0(_twobyone[pOrigin  ]._pad.x*1.e-6);
  double py0(_twobyone[pOrigin  ]._pad.y*1.e-6);
  double px1(_twobyone[pOrigin-1]._pad.x*1.e-6);
  double py1(_twobyone[pOrigin-1]._pad.y*1.e-6);
  _transform(a.xOrigin, a.yOrigin, px0, py0, D270);

  const double xAsicOffset = 1.0e-3;
  const double yAsicOffset = 1.0e-3;
  const double asicWidth   = 21.335e-3;
  //  const double asicHeight  = 22.488e-3;
#else
  const double xAsicOffset = 0;
  const double yAsicOffset = 0;
  const double asicWidth  = 110e-6*194; // 20.335e-3
  const double asicHeight = 110e-6*185;

  const double* v;
  v = reinterpret_cast<const double*>(_twobyone[pOrigin  ]._corner);
  double px0((v[0]+v[2]+v[4]+v[6])*0.25e-6);
  double py0((v[1]+v[3]+v[5]+v[7])*0.25e-6);
  v = reinterpret_cast<const double*>(_twobyone[pOrigin-1]._corner);
  double px1((v[0]+v[2]+v[4]+v[6])*0.25e-6);
  double py1((v[1]+v[3]+v[5]+v[7])*0.25e-6);
  _transform(a.xOrigin, a.yOrigin, px0, py0, D270);

  static const Rotation r2[] = { D270, D0, D90, D0 };
  _transform(a.xOrigin, a.yOrigin, -asicWidth, 0.5*asicHeight, r2[i]);
#endif

  double x0(xAsicOffset), y0(yAsicOffset);
  a.xAsicOrigin[0] = x0;
  a.yAsicOrigin[0] = y0;
  a.xAsicOrigin[1] = x0;
  a.yAsicOrigin[1] = y0+asicWidth;

  _transform(x0, y0, px1-px0, py1-py0, r[i]);

  a.xAsicOrigin[2] = x0;
  a.yAsicOrigin[2] = y0;
  a.xAsicOrigin[3] = x0;
  a.yAsicOrigin[3] = y0+asicWidth;

  return a;
}

//
//  Read alignment from corner locations and beam position
//  Rotate coordinates to be in x>0, y>0 quadrant
//
Ami::Cspad::QuadAlignment* QuadAlignment::load(FILE* g, bool offline)
{
  //    const double asicHeight = 110*194;
  const double asicWidth  = 110*185;

  Ami::Cspad::QuadAlignment q;
  Ami::Cspad::QuadAlignment* nq = new Ami::Cspad::QuadAlignment[4];

  double bx,by;

  size_t sz=256;
  char* linep = (char *)malloc(sz);
  char* pEnd;

  if (offline) {

    Ami::Cspad::QuadAlignment* pq = new Ami::Cspad::QuadAlignment[4];
    memset(pq, 0, 4*sizeof(Ami::Cspad::QuadAlignment));

    while(1) {
      if (getline(&linep, &sz, g)<0) break;
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
          pq[pindex]._twobyone[oindex]._pad.x += x0;
          pq[pindex]._twobyone[oindex]._pad.y += y0;
          for(unsigned c=0; c<4; c++) {
            pq[pindex]._twobyone[oindex]._corner[c].x += x0;
            pq[pindex]._twobyone[oindex]._corner[c].y += y0;
          }
        }
        else if (oname == "QUAD:V1") {
          double dx=0, dy=0;
          _transform(dx,dy,x0,y0,Rotation(unsigned(0.5+rot_z/90.)%NPHI));
          for(unsigned k=0; k<8; k++) {
            pq[pindex]._twobyone[k]._pad.x += x0;
            pq[pindex]._twobyone[k]._pad.y += y0;
            for(unsigned c=0; c<4; c++) {
              pq[oindex]._twobyone[k]._corner[c].x += dx;
              pq[oindex]._twobyone[k]._corner[c].y += dy;
            }
          }
        }
      }
    }

    //  Remap the 2x1s, if necessary
    for(unsigned iq=0; iq<4; iq++) {
      q = pq[iq];
      double qx=0, qy=0;
      for(unsigned i=0; i<8; i++) {
        qx += 0.125*q._twobyone[i]._pad.x;
        qy += 0.125*q._twobyone[i]._pad.y;
      }
      for(unsigned i=0; i<8; i++) {
        unsigned si=0;
        if (q._twobyone[i]._pad.x < qx) {
          if (q._twobyone[i]._pad.y + asicWidth < qy)
            si=0;
          else if (q._twobyone[i]._pad.y < qy)
            si=1;
          else if (q._twobyone[i]._pad.x + asicWidth < qx) 
            si=2;
          else
            si=3;
        }
        else {
          if (q._twobyone[i]._pad.y - asicWidth > qy)
            si=4;
          else if (q._twobyone[i]._pad.y > qy)
            si=5;
          else if (q._twobyone[i]._pad.x - asicWidth > qx) 
            si=7;
          else
            si=6;
        }
        nq[iq]._twobyone[si] = q._twobyone[i];
      }
    }
  }

  else {
    while(1) {
      getline(&linep, &sz, g);
      if (linep[0]!='#') {
        bx = strtod(linep,&pEnd);
        by = strtod(pEnd ,&pEnd);
        break;
      }
    }
  
    for(unsigned iq=0; iq<4; iq++) {
      double qx=0, qy=0;
      for(unsigned i=0; i<8; ) { // loop over 2x1's
        getline(&linep, &sz, g);
        if (linep[0]=='#') continue;
        double px = strtod(linep,&pEnd);
        double py = strtod(pEnd ,&pEnd);
        q._twobyone[i]._pad.x = px;
        q._twobyone[i]._pad.y = py;
        qx += 0.125*px;
        qy += 0.125*py;
        for(unsigned j=0; j<4; j++) {
          q._twobyone[i]._corner[j].x=px;
          q._twobyone[i]._corner[j].y=py;
        }
        i++;
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
        if (q._twobyone[i]._pad.x < qx) {
          if (q._twobyone[i]._pad.y + asicWidth < qy)
            si[i]=0;
          else if (q._twobyone[i]._pad.y < qy)
            si[i]=1;
          else if (q._twobyone[i]._pad.x + asicWidth < qx) 
            si[i]=2;
          else
            si[i]=3;
        }
        else {
          if (q._twobyone[i]._pad.y - asicWidth > qy)
            si[i]=4;
          else if (q._twobyone[i]._pad.y > qy)
            si[i]=5;
          else if (q._twobyone[i]._pad.x - asicWidth > qx) 
            si[i]=6;
          else
            si[i]=7;
        }
      }
    
      //     static const unsigned index[] = { 1,0,3,2,5,4,6,7,
      //                                       7,6,1,0,3,2,4,5,
      //                                       5,4,7,6,1,0,2,3,
      //                                       3,2,5,4,7,6,0,1 };
      static const unsigned index[] = { 0,1,2,3,4,5,7,6,
                                        2,3,4,5,7,6,0,1,
                                        4,5,7,6,0,1,2,3,
                                        7,6,0,1,2,3,4,5 };

      for(unsigned i=0; i<8; i++) {
        unsigned k = index[qr*8+si[i]];
        for(unsigned c=0; c<4; c++) {
          nq[iq]._twobyone[k]._corner[c].x = 0;
          nq[iq]._twobyone[k]._corner[c].y = 0;
          _transform(nq[iq]._twobyone[k]._corner[c].x,
                     nq[iq]._twobyone[k]._corner[c].y,
                     q._twobyone[i]._corner[c].x-bx,
                     q._twobyone[i]._corner[c].y-by,
                     qr);
        }
      }
    }
  }

  if (linep) {
    free(linep);
  }
  return nq;
}

