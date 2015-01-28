#include "ami/data/XML.hh"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

//#define DBUG

using namespace Ami::XML;

const int StringSize = 64;
const int IntSize = 16;
const int DoubleSize = 16;

static unsigned indent   = 0;
static bool     tag_open = false;

static char buff[8*1024];

TagIterator::TagIterator(const char*& p) :
  _p (p)
{
#ifdef DBUG
  const char* s = p;
  _tag = QtPersistent::extract_tag(p);
  printf("TagIterator %s [%p]\n",_tag.element.c_str(),s);
#else
  _tag = QtPersistent::extract_tag(p);
#endif
}

bool  TagIterator::end() const
{
  return _tag.name.empty();
}

TagIterator::operator const StartTag*() const
{
  return &_tag;
}

TagIterator& TagIterator::operator++(int)
{
  if (!end()) {
    const char* s;
    //  Find the stop tag
    std::string stop_tag;
    while(1) {
      s = _p;
      StartTag tag = QtPersistent::extract_tag(_p);
      if (tag.element[0]=='/') {
        stop_tag = tag.element.substr(1);
        break;
      }
      //  Found a new start tag
      //  Iterate through this level implicitly
      for(TagIterator it(s); !it.end(); it++) ;
      _p = s;
    }
#ifdef DBUG
    printf("TagIterator++ [%p] %s:%s\n",
           s,_tag.element.c_str(),stop_tag.c_str());
#endif
    if (stop_tag != _tag.element)
      printf("Mismatch tags %s/%s\n",_tag.element.c_str(),stop_tag.c_str());
    const char* p = _p;
    _tag = QtPersistent::extract_tag(p);
#ifdef DBUG
    printf("TagIterator++ %s [%p]\n",_tag.element.c_str(),_p);
#endif
    if (!end())
      _p = p;
  }
    
  return *this;
}

void QtPersistent::insert(char*& p, const StartTag& tag)
{
  *p++ = '\n';
  for(unsigned i=0; i<2*indent; i++)
    *p++ = ' ';

  p += sprintf(p, "<%s name=\"%s\">", tag.element.c_str(), tag.name.c_str());

  indent++;
  tag_open = true;
}

void QtPersistent::insert(char*& p, const StopTag& tag)
{
  indent--;
  if (!tag_open) {
    *p++ = '\n';
    for(unsigned i=0; i<2*indent; i++)
      *p++ = ' ';
  }

  p += sprintf(p, "</%s>", tag.element.c_str());

  tag_open = false;
}

void QtPersistent::insert(char*& p, const QString& s)
{
  //  printf("Inserting %s @ %p\n",qPrintable(s),p);
  int sz = s.size();
  for(int i=0; i<sz; i++) {
    const char c = s[i].toAscii();
    if (c && c!='<' && c!='&')
      *p++ = c;
    else
      p += sprintf(p,"&#x%04x",s[i].unicode());
  }
}

void QtPersistent::insert(char*& p, int s)
{
  p += sprintf(p,"%d",s);
}

void QtPersistent::insert(char*& p, unsigned s)
{
  p += sprintf(p,"0x%x",s);
}

void QtPersistent::insert(char*& p, double s)
{
  p += sprintf(p,"%g",s);
}

void QtPersistent::insert(char*& p, bool s)
{
  p += sprintf(p,"%s",s?"True":"False");
}

void QtPersistent::insert(char*& p, void* b, int len)
{
  char* end = (char*)b + len;
  for(char* bp = (char*)b; bp<end; bp++)
    p += sprintf(p, "%02hhx", *bp);
}

void QtPersistent::insert(char*& p, const double* s, unsigned n)
{
  for(unsigned i=0; i<n; i++)
    p += sprintf(p,"%g,",s[i]);
}

StartTag QtPersistent::extract_tag(const char*& p)
{
  while( *p++ != '<' ) ;

  StartTag tag;
  while( *p != ' ' && *p != '>')
    tag.element.push_back(*p++);
  if (*p++ != '>') {
    while( *p++ != '\"') ;
    while( *p != '\"' )
      tag.name.push_back(*p++);
    while( *p++ != '>') ;
  }

  if (*p == '\n') p++;

  return tag;
}

QString QtPersistent::extract_s(const char*& p)
{
  QString v;
  while(*p != '<') {
    if (*p != '&')
      v.append(*p++);
    else {
      p += 3; // skip &#x
      uint16_t u;
      sscanf(p, "%04hx", &u);
      v.append(u);
      p += 4;
    }
  }
  return v;
}

int QtPersistent::extract_i(const char*& p)
{
  char* endPtr;
  return strtol(p, &endPtr, 0);
  p = endPtr;
}

double QtPersistent::extract_d(const char*& p)
{
  char* endPtr;
  return strtod(p, &endPtr);
  p = endPtr;
}

bool QtPersistent::extract_b(const char*& p)
{
  QString v = QtPersistent::extract_s(p);
  return v.compare("True",::Qt::CaseInsensitive)==0;
}

void* QtPersistent::extract_op(const char*& p)
{
  char* b = buff;
  while(*p != '<') {
    sscanf(p, "%02hhx", b);
    p += 2;
    b++;
  }
  return buff;
}

unsigned QtPersistent::extract_d(const char*& p, double* s)
{
  char* endPtr;
  unsigned n=0;
  while(*p != '<') {
    s[n++] = strtod(p, &endPtr);
    p = endPtr+1;
  }
  return n;
}

