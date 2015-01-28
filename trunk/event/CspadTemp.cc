//-----------------------------------------------------------------------------
// File          : CxiTemp.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 01/26/2010
// Project       : LCLS - CXI Detector
//-----------------------------------------------------------------------------
// Description :
// Temperature Container
//-----------------------------------------------------------------------------
// Copyright (c) 2009 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 01/26/2010: created
//-----------------------------------------------------------------------------
#include <math.h>
#include "CspadTemp.hh"
using namespace std;

// Constants
static const double coeffA = -1.4141963e1;
static const double coeffB =  4.4307830e3;
static const double coeffC = -3.4078983e4;
static const double coeffD = -8.8941929e6;
static const double t25    = 10000.0;
static const double k0     = 273.15;
static const double vmax   = 3.3;
static const double vref   = 2.5;

// Temp range
static const double minTemp = -50;
static const double maxTemp = 150;
static const double incTemp = 0.01;

// Conversion table
static const unsigned int adcCnt = 4096;

#include <stdio.h>

// Constructor
CspadTemp::CspadTemp ( double rdiv ) :
  _rdiv(rdiv),
  _tempTable(new double[adcCnt]) 
{
   double       temp;
   double       tk;
   double       res;
   double       volt;
   unsigned int idx;

   temp = minTemp;
   while ( temp < maxTemp ) {
      tk = k0 + temp;
      res = t25 * exp(coeffA+(coeffB/tk)+(coeffC/(tk*tk))+(coeffD/(tk*tk*tk)));      
      volt = (res*vmax)/(_rdiv+res);
      idx = (int)((volt / vref) * (double)(adcCnt-1));
      if ( idx < adcCnt ) _tempTable[idx] = temp; 
      temp += incTemp;
   }
}

CspadTemp::~CspadTemp() {
  delete[] _tempTable;
}

// Get Resistance from adc value
double CspadTemp::getResist (unsigned adcValue) const
{
   double v;
   if ( adcValue < adcCnt) {
      v = getVoltage(adcValue);
      return((v * _rdiv) / (vmax - v));
   }
   else return(0);
}


// Get Voltage from adc value
double CspadTemp::getVoltage (unsigned adcValue) const
{
   if ( adcValue < adcCnt) return(((double)adcValue / (double)(adcCnt-1)) * vref);
   else return(0);
}


// Get Temperature from adc value, deg C
double CspadTemp::getTemp (unsigned adcValue) const
{
   if ( adcValue < adcCnt) return(_tempTable[adcValue]);
   else return(0);
}

