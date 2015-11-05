#include "QtTree.hh"

#include "ami/data/QtPersistent.hh"

#include <QtGui/QPalette>

#include <stdio.h>

//#define DBUG

using namespace Ami::Qt;

static QStringList _most_recent;
const int MOST_RECENT_DISPLAY = 8;
const int MOST_RECENT_SIZE    = 10;
static QStringList _favorites;

//
//  Split 'str' at occurrences of 'sep'.  Keep 'sep' at trail of pieces.
//
static QStringList split(const QString& str,
                         const QString& sepstr,
                         int            level,
                         int            length=-1)
{
  QRegExp sep(sepstr);
  QStringList l = str.split(sep,QString::SkipEmptyParts).mid(level,length);
  int pos=0;
  for(int i=0; i<l.size(); i++) {
    QString& s = l[i];
    pos = str.indexOf(s, pos) + s.size();
    if (pos < str.size())
      s.append(str.at(pos));
  }
  return l;
}

#ifdef DBUG
static void dump_list(const char* ttl,
                      const QString& name,
                      const QStringList& name_f)
{ 
  printf("%s [%s]:", ttl, qPrintable(name));
  for(int i=0; i<name_f.size(); i++)
    printf(" [%s]", qPrintable(name_f[i]));
  printf("\n"); 
}
#endif

static void push(QStandardItem& root, 
                 const QString& name, 
                 int level,
                 const QString& separator)
{
  QStringList name_f = split(name,separator,level);
#ifdef DBUG
  dump_list("push",name,name_f);
#endif
  if (name_f.size()==0) {
    root.insertRow(0, new QStandardItem(name) );
    return;
  }

  for(int i=0; i<root.rowCount(); i++) {
    QStandardItem& row = *root.child(i);
    QStringList row_f = split(row.text(),separator,level);
#ifdef DBUG
    dump_list("push row",row.text(),row_f);
#endif    
    //  If they share any leading fields:
    if (name_f.size() && row_f.size() && 
        name_f[0]==row_f[0] &&
        row.text()!=root.text()) {

      //  If they are equal upto the row's length:
      if (name_f.size()>=row_f.size() &&
          name_f.mid(0,row_f.size()) == row_f) {
        //  If the row has children:
        if (row.rowCount()) {
          //  push onto the row's tree
          push(row, name, level+row_f.size(),separator);
        }
        else {
          //  Create a tree from the row and append
          row.appendRow( new QStandardItem(row.text()) );
          row.appendRow( new QStandardItem(name) );
        }
      }
      else {
        //  Create a new root with these two children
        QStringList common = split(name,separator,0,level);
#ifdef DBUG
        dump_list("push common in",name,common);
#endif
        //        while( name_f.size() && row_f.size() && name_f[0] == row_f[0] ) {
        while( name_f.size()>1 && row_f.size() && name_f[0] == row_f[0] ) {
          common.append( name_f.takeFirst() );
          row_f.takeFirst();
        }
        //        QStandardItem* branch = new QStandardItem( common.join(separator) );
        QString common_branch = common.join("");
#ifdef DBUG
        printf("common_branch [%s]\n",qPrintable(common_branch));
        dump_list("common name",name,name_f);
        dump_list("common row ",row.text(),row_f);
#endif
        QStandardItem* branch = new QStandardItem( common_branch );
        root.takeRow(i);
        root.insertRow( i, branch );
        branch->appendRow( &row );
        QStandardItem* leaf = new QStandardItem( name );
        if (name_f[0] < row_f[0]) {
          branch->insertRow(0, leaf);
        }
        else {
          branch->appendRow(leaf);
        }
      }
    }
    //  Insert before or after
    else if (row_f.size() && name_f[0] < row_f[0]) {
      root.insertRow(i, new QStandardItem(name));
    }
    else
      continue;
    return;
  }

  root.appendRow(new QStandardItem(name));
}

QtTree::QtTree(const QString& separator) :
  QPushButton(),
  _view      (this),
  _entry     (),
  _separator (separator),
  _use_scan  (true)
{
  _view.setWindowFlags(::Qt::Dialog);
  _view.setWindowModality(::Qt::NonModal);

  _view.setModel(&_model);

  connect(this  , SIGNAL(clicked()), this, SLOT(show_tree()));
  connect(&_view, SIGNAL(clicked(const QModelIndex&)), this, SLOT(set_entry(const QModelIndex&)));
}

QtTree::QtTree(const QStringList& names, 
               const QStringList& help, 
               const QColor& color,
               const QString& separator,
               const QStringList& pnames) :
  QPushButton(),
  _view      (this),
  _entry     (),
  _separator (separator),
  _use_scan  (true)
{

  QPalette newPalette = palette();
  newPalette.setColor(QPalette::Button, color);
  setPalette(newPalette);

  _view.setWindowFlags(::Qt::Dialog);
  _view.setWindowModality(::Qt::NonModal);
  //  _view.setWindowFlags(::Qt::Popup);

  _view.setModel(&_model);

  fill(names);

  connect(this    , SIGNAL(clicked()), this, SLOT(show_tree()));
  connect(&_view  , SIGNAL(clicked(const QModelIndex&)), this, SLOT(set_entry(const QModelIndex&)));
}

void QtTree::fill(const QStringList& names)
{
  QStringList scan_names;
  QStringList self_names;

  if (names.size()) {
    QStandardItem* root = _model.invisibleRootItem();

    bool lFirst=true;
    for(int i=0; i<names.size(); i++) {
      QString n(names.at(i));
      if (n.contains("SELF:"))
        self_names.push_back(n);
      else if (n.contains("(SCAN)"))
        scan_names.push_back(n);
      else if (lFirst) {
        lFirst=false;
        root->appendRow( new QStandardItem( n ) );
      }
      else
        push(*root,n,0,_separator);
    }
    for(int i=0; i<scan_names.size(); i++)
      root->insertRow(0, new QStandardItem(scan_names.at(i)));

    for(int i=0; i<self_names.size(); i++)
      root->insertRow(0, new QStandardItem(self_names.at(i)));

    for(int i=0; i<root->rowCount(); i++) {
      QStandardItem* ch = root->child(i);
      if (ch->text().startsWith("Post")) {
        root->insertRow(0, root->takeRow(i));
        break;
      }
    }

    root->insertRow( 0, new QStandardItem( QString("[Favorites]")) );
    root->insertRow( 0, new QStandardItem( QString("[Most Recent]")) );
  }

  _view.reset();
  if (_use_scan && scan_names.size())
    set_entry(scan_names.at(0));
  else
    set_entry(_entry);
}

QtTree::~QtTree()
{
}

void QtTree::save(char*& p) const
{
  XML_insert( p, "QString", "_entry",
              QtPersistent::insert(p, _entry) );
}

void QtTree::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.element == "QString")
      set_entry(QtPersistent::extract_s(p));
  XML_iterate_close(QtTree,tag);
}

void QtTree::save_preferences(char*& p)
{
  if (_most_recent.size()) {
    for(int i=0; i<_most_recent.size(); i++)
      XML_insert( p, "QString", "recent", QtPersistent::insert(p,_most_recent[i]));
  }
  else
    XML_insert( p, "Nothing", "name", QtPersistent::insert(p, "nil"));
}

void QtTree::load_preferences(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.name == "recent") {
      _most_recent.push_back(QtPersistent::extract_s(p));
    }
  XML_iterate_close(QtTree,tag);
}

void QtTree::load_favorites(char* p)
{
  char* s;
  char* arg = strtok_r(p,"\t\n ",&s);
  while(arg) {
    _favorites.push_back(QString(arg));
    arg = strtok_r(NULL,"\t\n ",&s);
  }
}

const QString& QtTree::entry() const { return _entry; }

void QtTree::clear()
{
  QStandardItem* root = _model.invisibleRootItem();
  root->removeRows(0,root->rowCount());
  _view.reset();
}

void QtTree::set_entry(const QModelIndex& e) { 
  const QString& entry = _model.itemFromIndex(e)->text();
  set_entry(entry);
  emit activated(entry);
}

void QtTree::set_entry(const QString& e) { 
  QPalette p(palette());
  if (_valid_entry(e)) {
    p.setColor(QPalette::ButtonText, QColor(0,0,0));
    _view.hide();

    if (!e.isEmpty()) {
      if (_model.invisibleRootItem()->child(0)) {
        QStandardItem& mru_root = *_model.invisibleRootItem()->child(0);
        for(int i=0; i<mru_root.rowCount(); i++) {
          QStandardItem& row = *mru_root.child(i);
          if (row.text() == e) {
            mru_root.takeRow(i);
            break;
          }
        }
        mru_root.insertRow(0, new QStandardItem(e));
        while (mru_root.rowCount() > MOST_RECENT_DISPLAY)
          mru_root.takeRow(MOST_RECENT_DISPLAY);
    
        _most_recent.removeAll(e);
        _most_recent.push_front(e);
        while (_most_recent.size() > MOST_RECENT_SIZE)
          _most_recent.pop_back();
      }
    }
  }
  else
    p.setColor(QPalette::ButtonText, QColor(0xc0,0,0));

  this->setText(_entry=e);
  this->setPalette(p);
}

static bool has_child_named(const QStandardItem& root, 
                            const QString& name)
{
  for(int i=0; i<root.rowCount(); i++) {
    const QStandardItem& row = *root.child(i);
    if (row.text()==name || has_child_named(row,name))
      return true;
  }
  return false;
}

void QtTree::show_tree()
{
  _model.invisibleRootItem()->removeRow(0);
  _model.invisibleRootItem()->removeRow(0);

  QStandardItem& fav_root = *new QStandardItem( QString("[Favorites]"));
  _model.invisibleRootItem()->insertRow(0, &fav_root);

  for(int i=0; i<_favorites.size(); i++) {
    if (has_child_named(*_model.invisibleRootItem(), _favorites[i]))
      fav_root.appendRow( new QStandardItem(_favorites[i]) );
  }
  
  QStandardItem& mru_root = *new QStandardItem( QString("[Most Recent]"));
  _model.invisibleRootItem()->insertRow(0, &mru_root);

  for(int i=0,n=0; i<_most_recent.size(); i++) {
    if (has_child_named(*_model.invisibleRootItem(), _most_recent[i])) {
      mru_root.appendRow( new QStandardItem(_most_recent[i]) );
      if (++n >= MOST_RECENT_DISPLAY)
        break;
    }
  }

  _view.show();
}

bool QtTree::_valid_entry(const QString&) const { return true; }

void QtTree::use_scan(bool l) { _use_scan=l; }

