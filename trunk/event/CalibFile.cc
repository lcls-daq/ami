#include "ami/event/CalibFile.hh"

#include <boost/lexical_cast.hpp>

using namespace Ami;

CalibFile::CalibFile(const std::string& path) 
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
    
