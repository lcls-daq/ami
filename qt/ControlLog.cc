#include "ControlLog.hh"

#include <QtCore/QString>
#include <QtCore/QTime>
#include <QtGui/QTextCursor>

using namespace Ami::Qt;

static ControlLog* _instance = 0;

ControlLog::ControlLog() :
  QTextEdit(QString())
{
  setReadOnly(true);
  qRegisterMetaType<QTextCursor>();
  QObject::connect(this, SIGNAL(appended(const QString&)),
		   this, SLOT(append(const QString&)));
  appendText(QString("Ami client started"));
}

ControlLog::~ControlLog() {}

void ControlLog::appendText(const QString& t) 
{
  QString s = QString("%1: %2")
    .arg(QTime::currentTime().toString("hh:mm:ss"))
    .arg(t);
			  
  emit appended(s); 
}

ControlLog& ControlLog::instance()
{
  if (!_instance)
    _instance = new ControlLog;
  return *_instance;
}
