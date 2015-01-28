#ifndef Ami_CalibFile_hh
#define Ami_CalibFile_hh
//
// helper class for ordering files in a directory
// copy from /reg/g/psdm/sw/releases/current/PSCalib/src/CalibFileFinder.cpp
//   minus some boost
//
#include <string>

namespace Ami {
  class CalibFile {
  public:
    CalibFile(const std::string& path);
    
    const std::string path() const { return m_path; }
    unsigned begin() const { return m_begin; }
    unsigned end() const { return m_end; }

    // comparison for sorting
    bool operator<(const CalibFile& other) const {
      if (m_begin != other.m_begin) return m_begin < other.m_begin;
      return m_end > other.m_end;
    }
    
  private:
    std::string m_path;
    unsigned m_begin;
    unsigned m_end;
  };
};

#endif
