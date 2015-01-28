#ifndef QtXML_hh
#define QtXML_hh

#include <map>

/*
**  Tools for saving GUI setup to an XML file
**
**    GUI screen data is typically saved by the data type ("element")
**    and member name ("name") of the class data members for each of the 
**    screens and their associated plotting classes.
**
**    See also QtPersistent.hh.
*/

namespace Ami {
  namespace XML {
    class StartTag {
    public:
      StartTag() {}
      StartTag(const std::string element_, const std::string name_) :
        element(element_), name   (name_) {}

    public:
      std::string element;
      std::string name;
    };

    class StopTag {
    public:
      StopTag(const std::string element_) : element(element_) {}
    public:
      const std::string element;
    };

    class TagIterator {
    public:
      TagIterator(const char*&);
    public:
      bool            end() const;
      operator  const StartTag*() const;
      TagIterator&    operator++(int);
    private:
      const char*& _p;
      StartTag     _tag;
    };
  };
}

/**
 **  Convenience macros for reading XML data.
 **  Typical use (from within class "XClass"):
 **
 **    XML_iterate_open(stream, tag)
 **      if   (tag.element == "some-data-type")
 **        _data_member = QtPersistent::extract_type(stream);
 **      else if (tag...)
 **    XML_iterate_close("XClass", tag);
 **
 **/
#define XML_iterate_open(pvar,tvar)                      \
  for(Ami::XML::TagIterator it(pvar); !it.end(); it++) { \
    const Ami::XML::StartTag& tvar = *it;                

/**
 **  Validates match of close tag with previous open tag
 **/
#define XML_iterate_close(where,tvar)           \
  else                                          \
    printf(#where " unknown tag %s/%s\n",       \
           tvar.element.c_str(),                \
           tvar.name.c_str());                  \
  }

#endif
