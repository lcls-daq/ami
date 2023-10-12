#include "BlackHole.hh"

#include <fstream>

using namespace Ami;

void BlackHole::_dump(ndarray<const data_t, 3> data) {
  if (_ndump>MAXDUMP) return;
  std::ofstream file;
  char fname[80];
  // unfortunately the event time and fiducials are
  // not available in AMI plugins
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  sprintf(fname,"/tmp/cspadtripper_%ld_%ld_%d.dat",now.tv_sec,
          now.tv_nsec,_ndump);
  file.open (fname, std::ios::out | std::ios::binary);
  if (!file.is_open()) return;
  file.write((char*)data.data(),data.size()*sizeof(data_t));
  file.close();
  _ndump++;
}

bool BlackHole::_maybePush(unsigned seg, unsigned r, unsigned c,
                           const ndarray<const data_t, 3>& data) {
  // check for unassigned pixel
  if (_conmap(r,c)==-1) {
    if (data(seg,r,c)>_thresh) {
      _conmap(r,c)=0; // above threshold
    }
    else {
      _stack.push_back(Point(r,c)); // connected pixel
      return true;
    }
  }
  return false;
}

void BlackHole::setupGoodPix(ndarray<const data_t, 3> data, uint quad,
                             unsigned currentDet) {
  if (_goodpix.empty()) {
    printf("*** make array\n");
    _goodpix = make_ndarray<data_t>(_detsMax*Pds::CsPad::MaxQuadsPerSensor,
                                    data.shape()[0],data.shape()[1],
                                    data.shape()[2]);
  }

  unsigned nbadpix=0;
  for(unsigned s = 0; s<data.shape()[0]; s++) {
    for(unsigned r = 0; r<data.shape()[1]; r++) {
      for(unsigned c = 0; c<data.shape()[2]; c++) {
        if (data(s,r,c)<(_thresh-1000)) {
          _goodpix(currentDet*Pds::CsPad::MaxQuadsPerSensor+quad,s,r,c) = 1;
        } else {
          _goodpix(currentDet*Pds::CsPad::MaxQuadsPerSensor+quad,s,r,c) = 0;
          nbadpix++;
        }
      }
    }
  }
  printf("--- Found %d initial badpix in quad %d\n",nbadpix,quad);
}

void BlackHole::updateGoodPix(ndarray<const data_t, 3> data, uint quad,
                              unsigned currentDet) {
  unsigned nbadpix=0;
  for(unsigned s = 0; s<data.shape()[0]; s++) {
    for(unsigned r = 0; r<data.shape()[1]; r++) {
      for(unsigned c = 0; c<data.shape()[2]; c++) {
        // check to see if the pixel is bad for two events in a row
        if (data(s,r,c)<(_thresh-1000) && !_goodpix(currentDet*Pds::CsPad::MaxQuadsPerSensor+quad,s,r,c)) {
          _goodpix(currentDet*Pds::CsPad::MaxQuadsPerSensor+quad,s,r,c) = 1;
        }
        if (!_goodpix(currentDet*Pds::CsPad::MaxQuadsPerSensor+quad,s,r,c)) nbadpix++;
      }
    }
  }
  printf("*** found %d bad pixels in quad %d\n",nbadpix,quad);
}

bool BlackHole::floodFill(ndarray<const data_t, 3> data, uint quad, unsigned currentDet) {
  for(unsigned sec = 0; sec<data.shape()[0]; sec++) {
    _numreg=0;
    std::fill_n(_conmap.data(), int(_conmap.size()), conmap_t(-1));
    unsigned nrows = data.shape()[1];
    unsigned ncols = data.shape()[2];
    unsigned nabovethresh = 0;
    for(unsigned row = 0; row<nrows; row++) {
      for(unsigned col = 0; col<ncols; col++) {
        if (data(sec,row,col)>_thresh && _goodpix(currentDet*Pds::CsPad::MaxQuadsPerSensor+quad,sec,row,col)) nabovethresh++;
      }
    }
    // don't bother with flood-fill if we don't have enough
    // pixels above threshold
    if (nabovethresh<_holeNpixMin+10) continue;
    printf("*** Found %d above thresh in quad %d section %d\n",nabovethresh, quad, sec);
    for(unsigned row = 0; row<nrows; row++) {
      for(unsigned col = 0; col<ncols; col++) {

        if (_maybePush(sec,row,col,data)) {
          ++ _numreg; // start a new region
        } else {
          continue;
        }
        while (!_stack.empty()) {
          Point val = _stack.back();
          _stack.pop_back();
          unsigned r = val.r();
          unsigned c = val.c();
          _conmap(r,c) = _numreg;

          if (r>0)       _maybePush(sec,r-1,c,data);
          if (r<nrows-1) _maybePush(sec,r+1,c,data);
          if (c>0)       _maybePush(sec,r,c-1,data);
          if (c<ncols-1) _maybePush(sec,r,c+1,data);
        }
      }
    }
    bool trip = _trip();
    if (trip) {
      _lasttrip_sec = sec;
      if (_nprint<MAXPRINT) {
        printf("*** Found blackhole with %d pixels at time %lu\n",
               lasttrip_pixcount(),(unsigned long)time(NULL));
        _nprint++;
      }
      _dump(data);
      return true;
    }
  }
  return false;
}

bool BlackHole::_trip() {
  std::fill_n(_count, MAXREGION, unsigned(0));
  std::fill_n(_edge, MAXREGION, bool(0));

  for(unsigned r = 0; r<_conmap.shape()[0]; r++) {
    for(unsigned c = 0; c<_conmap.shape()[1]; c++) {
      conmap_t region = _conmap(r,c);
      if (_conmap(r,c)==0) continue;
      if (region==0) continue; // ignore above-threshold pixels
      region -= 1; // to count from zero
      if (region >= MAXREGION) continue; // shouldn't happen
      _count[region]++;
      // ignore any region that has an edge in it (i.e. not
      // completely enclosed)
      if (!_edge[region]) {
        _edge[region] = (r==0) || (r==_conmap.shape()[0]-1) ||
          (c==0) || (c==_conmap.shape()[1]-1);
      }
    }
  }

  for (unsigned i=0; i<_numreg; i++) {
    // trip if we have correct-number of below-threshold pixels
    // not at an edge (we could imagine removing the edge requirement)
    if (_count[i]>=_holeNpixMin && _count[i]<=_holeNpixMax && !_edge[i]) {
      _lasttrip_pixcount = _count[i];
      return true;
    }
  }
  return false;
}
