#ifndef AmiQt_ControlLog_hh
#define AmiQt_ControlLog_hh

#include <QtGui/QTextEdit>
#include <QtGui/QTextCursor>

namespace Ami {
  namespace Qt {
    class ControlLog : public QTextEdit {
      Q_OBJECT
    public:
      static ControlLog& instance();
    private:
      ControlLog();
      ~ControlLog();
    public:
      void appendText(const QString&);
      signals:
      void appended(QString);
    };
  };
};

Q_DECLARE_METATYPE(QTextCursor)

#endif
