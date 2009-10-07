#ifndef AmiQt_FileSelect_hh
#define AmiQt_FileSelect_hh

#include <QtGui/QWidget>

class QString;
class QDate;
class QDateEdit;
class QListWidget;
class QPushButton;

namespace Ami {
  namespace Qt {
    class FileSelect : public QWidget {
      Q_OBJECT
    public:
      FileSelect(QWidget* parent,
		 const QStringList& paths);
      ~FileSelect();
    public:
      QStringList paths() const;
    public slots:
      void change_date(const QDate&);
      void change_path_list(const QStringList&);
      void file_selected(int);
    private:
      QStringList  _paths;
      QString      _file;
      QListWidget* _list;
      QDateEdit*   _date;
    };
  };
};

#endif
