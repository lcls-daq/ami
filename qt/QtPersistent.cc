#include "ami/qt/QtPersistent.hh"

using namespace Ami::Qt;

const int StringSize = 64;
const int IntSize = 16;
const int DoubleSize = 16;

void QtPersistent::insert(char*& p, const QString& s)
{
  //  printf("Inserting %s @ %p\n",qPrintable(s),p);

  int sz = s.size();
  *reinterpret_cast<int*>(p) = sz;
  p += sizeof(int);
  memcpy(p,s.constData(),sz*sizeof(QChar));
  p += sz*sizeof(QChar);
}

void QtPersistent::insert(char*& p, int s)
{
  char buff[IntSize];
  sprintf(buff,"%d",s);
  strncpy(p,buff,IntSize);
  p += IntSize;
}

void QtPersistent::insert(char*& p, unsigned s)
{
  char buff[IntSize];
  sprintf(buff,"%d",s);
  strncpy(p,buff,IntSize);
  p += IntSize;
}

void QtPersistent::insert(char*& p, double s)
{
  char buff[DoubleSize];
  sprintf(buff,"%g",s);
  strncpy(p,buff,DoubleSize);
  p += DoubleSize;
}

void QtPersistent::insert(char*& p, bool s)
{
  char buff[IntSize];
  sprintf(buff,"%d",s ? 1:0);
  strncpy(p,buff,IntSize);
  p += IntSize;
}

QString QtPersistent::extract_s(const char*& p)
{
  //const char* t = p;
  int sz = *reinterpret_cast<const int*>(p);
  p += sizeof(int);
  const QChar* ch = reinterpret_cast<const QChar*>(p);
  p += sz*sizeof(QChar);
  QString s(ch,sz);
  //  printf("Extracted %s @ %p\n",qPrintable(s),t);
  return s;
}

int QtPersistent::extract_i(const char*& p)
{
  char buff[IntSize];
  strncpy(buff, p, IntSize);
  p += IntSize;
  return atoi(buff);
}

double QtPersistent::extract_d(const char*& p)
{
  char buff[DoubleSize];
  strncpy(buff, p, DoubleSize);
  p += DoubleSize;
  return strtod(buff,NULL);
}

bool QtPersistent::extract_b(const char*& p)
{
  char buff[IntSize];
  strncpy(buff, p, IntSize);
  p += IntSize;
  return atoi(buff)==0 ? false : true;
}
