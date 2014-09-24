#ifndef AmiXML_hh
#define AmiXML_hh

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

#include <QtCore/QString>

namespace Ami {
  namespace XML {
    class StartTag;
    class StopTag;

    class QtPersistent {
    public:
      virtual ~QtPersistent() {}
    public:
//       virtual void save(char*& p) const = 0;
//       virtual void load(const char*& p) = 0;
    public:
      /** 
       **  insert data of the named type into a char-stream
       **/
      static void insert(char*&, const StartTag&);
      static void insert(char*&, const StopTag&);
      static void insert(char*&, const QString&);
      static void insert(char*&, unsigned);
      static void insert(char*&, int);
      static void insert(char*&, double);
      static void insert(char*&, bool);
      static void insert(char*&, void*, int);
      static void insert(char*&, const double*, unsigned);  // array
      /**
       **  extract data of the return type from the char-stream
       **/
      static Ami::XML::StartTag extract_tag(const char*&);
      static int      extract_i(const char*&);
      static double   extract_d(const char*&);
      static QString  extract_s(const char*&);
      static bool     extract_b(const char*&);
      static void*    extract_op(const char*&);
      static unsigned extract_d(const char*&,double*);  // array
    };

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

/**
 **  Helper macro for convenience.
 **  Typical use:
 **    XML_insert(stream,"data-type", "member-name", QtPersistent::insert());
 **/
#define XML_insert( p, element, name, routine ) {               \
    QtPersistent::insert(p, Ami::XML::StartTag(element,name));  \
    { routine; }                                                \
    QtPersistent::insert(p, Ami::XML::StopTag (element)); }

#endif
