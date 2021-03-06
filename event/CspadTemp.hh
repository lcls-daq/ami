//-----------------------------------------------------------------------------
// File          : CxiTemp.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 01/26/2009
// Project       : LCLS - CXI Detector
//-----------------------------------------------------------------------------
// Description :
// Temperature Container
//-----------------------------------------------------------------------------
// Copyright (c) 2009 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 01/26/2009: created
//-----------------------------------------------------------------------------
#ifndef __CXI_TEMP_H__
#define __CXI_TEMP_H__

// Calibration Data Class
class CspadTemp {

public:

  // Constructor
  CspadTemp (double rdiv);
  // Destructor
  ~CspadTemp();

  // Get Resistance from adc value
  double getResist (unsigned) const;

  // Get Voltage from adc value
  double getVoltage (unsigned) const;

  // Get Temperature from adc value, deg C
  double getTemp (unsigned) const;

private:
  double  _rdiv;
  double* _tempTable;
};

#endif
