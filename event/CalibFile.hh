#ifndef Ami_CalibFile_hh
#define Ami_CalibFile_hh
//
// helper class for ordering files in a directory
// copy from /reg/g/psdm/sw/releases/current/PSCalib/src/CalibFileFinder.cpp
//   minus some boost
//
#include <string>
#include <boost/lexical_cast.hpp>

namespace Ami {
  class CalibFile {
  public:
    CalibFile(const std::string& path) 
      : m_path(path)
    {
      std::string basename = ::basename(path.c_str());
      std::string::size_type p = basename.find('-');
      if (p == std::string::npos) { 
        throw std::string("missing dash in filename: " + path);
      }
      
      std::string beginstr(basename, 0, p);
      std::string endstr(basename, p+1);
      endstr = endstr.substr(0,endstr.find('.'));
      
      m_begin = boost::lexical_cast<unsigned>(beginstr);
      if (endstr == "end") {
        m_end = std::numeric_limits<unsigned>::max();
      } else {
        m_end = boost::lexical_cast<unsigned>(endstr);
      }      
    }
    
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
