#ifndef AmiQt_QtTree_hh
#define AmiQt_QtTree_hh

#include <QtGui/QTreeView>
#include <QtGui/QPushButton>

#include <QtGui/QStandardItemModel>
#include <QtCore/QStringList>

class QColor;

namespace Ami {
  namespace Qt {
    class QtTree : public QPushButton {
      Q_OBJECT
    public:
      QtTree(const QString& separator);
      QtTree(const QStringList&, const QStringList&, const QColor&,
             const QString& separator,
             const QStringList& pnames=QStringList());
      virtual ~QtTree();
    public:
      void save(char*&) const;
      void load(const char*&);
    public:
      const QString& entry() const;
      void  clear();
      void  fill (const QStringList&);
      void  use_scan(bool);
    public slots:
      void set_entry(const QModelIndex&);
      void set_entry(const QString&);
      void show_tree();
    signals:
      void activated(const QString&);
    private:
      virtual bool _valid_entry(const QString&) const;
    protected:
      QStandardItemModel _model;
      QTreeView          _view;
      QString            _entry;
      QString            _separator;
      bool               _use_scan;
    };
  };
};

#endif
