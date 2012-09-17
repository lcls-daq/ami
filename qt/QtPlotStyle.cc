#include "ami/qt/QtPlotStyle.hh"
#include "ami/qt/QtPersistent.hh"

#include <QtGui/QPainter>
#include <QtGui/QLineEdit>
#include <QtGui/QPen>
#include <QtGui/QIntValidator>
#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QPushButton>

#include <QtGui/QDialog>
#include "qwt_symbol.h"
#include <vector>


namespace Ami {
  namespace Qt {
    class StyleDialog : public QDialog {
    public:
      StyleDialog(QWidget* p, 
                  unsigned ssize,
                  unsigned sstyle,
                  unsigned lsize,
                  unsigned lstyle);
      ~StyleDialog();
    public:
      unsigned symbolSize() const;
      unsigned symbolStyle() const;
      unsigned lineSize() const;
      unsigned lineStyle() const;
    private:
      void _update_symbols();
      void _update_lines  ();
    private:
      QLineEdit* _ssize;
      QComboBox* _sstyle;
      std::vector< ::QwtSymbol::Style > _index_sstyle;
      QLineEdit* _lsize;
      QComboBox* _lstyle;
      std::vector< ::Qt::PenStyle > _index_lstyle;
    };
  };
};

using namespace Ami::Qt;

StyleDialog::StyleDialog(QWidget* p, 
                         unsigned ssize,
                         unsigned sstyle,
                         unsigned lsize,
                         unsigned lstyle) : QDialog(p) 
{
  setWindowTitle("Style");

  _ssize = new QLineEdit(QString::number(ssize));
  _ssize->setMaximumWidth(30);
  new QIntValidator(1,99,_ssize);

  _sstyle = new QComboBox;
  _update_symbols();
  for(unsigned i=0; i<_index_sstyle.size(); i++)
    if (sstyle == unsigned(_index_sstyle[i]))
      _sstyle->setCurrentIndex(i);
    
  _lsize = new QLineEdit(QString::number(lsize));
  _lsize->setMaximumWidth(30);
  new QIntValidator(1,99,_lsize);

  _lstyle = new QComboBox;
  _update_lines();
  for(unsigned i=0; i<_index_lstyle.size(); i++)
    if (lstyle == unsigned(_index_lstyle[i]))
      _lstyle->setCurrentIndex(i);

  QPushButton* acceptB = new QPushButton("OK");

  QGridLayout* l = new QGridLayout;
  l->addWidget(_ssize ,0,0);
  l->addWidget(_sstyle,0,1);
  l->addWidget(_lsize ,1,0);
  l->addWidget(_lstyle,1,1);
  l->addWidget(acceptB,2,0,1,2);
  setLayout(l);

  connect( acceptB, SIGNAL(clicked()), this, SLOT(accept()) );
}

StyleDialog::~StyleDialog()
{
}

unsigned StyleDialog::symbolSize() const 
{
  return _ssize->text().toUInt();
}

unsigned StyleDialog::symbolStyle() const 
{
  return unsigned(_index_sstyle[_sstyle->currentIndex()]);
}

unsigned StyleDialog::lineSize() const 
{
  return _lsize->text().toUInt();
}

unsigned StyleDialog::lineStyle() const 
{
  return unsigned(_index_lstyle[_lstyle->currentIndex()]);
}

void StyleDialog::_update_symbols()
{
#define ADD_STYLE(s) {                                  \
    QPixmap map(20,20);                                 \
    map.fill(QColor(255,255,255));                      \
    QPainter p(&map);                                   \
    QwtSymbol sym(::QwtSymbol::s,qbrush,qpen,qsize);    \
    sym.draw(&p,10,10);                                 \
    _sstyle->addItem(QIcon(map),"");                    \
    _index_sstyle.push_back(::QwtSymbol::s);            \
  }

  _sstyle->clear();

  unsigned ssize = 10;

  QColor qcol(0,0,0);
  QBrush qbrush(qcol);
  QPen   qpen(qcol);
  QSize  qsize(ssize,ssize);
  ADD_STYLE(Ellipse);
  ADD_STYLE(Rect);
  ADD_STYLE(Diamond);
  ADD_STYLE(Triangle);
  ADD_STYLE(Cross);
  ADD_STYLE(XCross);
#undef ADD_STYLE
}

void StyleDialog::_update_lines()
{
#define ADD_STYLE(s) {                                  \
    QPixmap map(20,20);                                 \
    map.fill(QColor(255,255,255));                      \
    QPainter p(&map);                                   \
    qpen.setStyle(::Qt::s);                             \
    p.setPen(qpen);                                     \
    p.drawLine(2,10,18,10);                             \
    _lstyle->addItem(QIcon(map),"");                    \
    _index_lstyle.push_back(::Qt::s);                   \
  }

  _lstyle->clear();

  unsigned ssize = 5;

  QColor qcol(0,0,0);
  QBrush qbrush(qcol);
  QPen   qpen(qcol);
  QSize  qsize(ssize,ssize);
  ADD_STYLE(SolidLine);
  ADD_STYLE(DashLine);
  ADD_STYLE(DotLine);
  ADD_STYLE(DashDotLine);
  ADD_STYLE(DashDotDotLine);
#undef ADD_STYLE
}

static const int Initial_symbol_size  = 0;
static const int Default_symbol_size  = 5;
static const int Default_symbol_style = int(QwtSymbol::Diamond);
static const int Default_line_size    = 1;
static const int Default_line_style   = int(::Qt::SolidLine);


QtPlotStyle::QtPlotStyle() :
  _symbol_size (Initial_symbol_size),
  _symbol_style(Default_symbol_style),
  _line_size   (Default_line_size),
  _line_style  (Default_line_style)
{
}

QtPlotStyle::QtPlotStyle(const QtPlotStyle& p) :
  _symbol_size (p._symbol_size),
  _symbol_style(p._symbol_style),
  _line_size   (p._line_size),
  _line_style  (p._line_style)
{
}

QtPlotStyle::~QtPlotStyle()
{
}

void QtPlotStyle::save(char*& p) const
{
  XML_insert(p, "int" , "_symbol_size"     , QtPersistent::insert(p,_symbol_size) );
  XML_insert(p, "int" , "_symbol_style"    , QtPersistent::insert(p,_symbol_style) );
  XML_insert(p, "int" , "_line_size"       , QtPersistent::insert(p,_line_size) );
  XML_insert(p, "int" , "_line_style"      , QtPersistent::insert(p,_line_style) );
}

void QtPlotStyle::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.name == "_symbol_size")
      _symbol_size = QtPersistent::extract_i(p);
    else if (tag.name == "_symbol_style")
      _symbol_style = QtPersistent::extract_i(p);
    else if (tag.name == "_line_size")
      _line_size = QtPersistent::extract_i(p);
    else if (tag.name == "_line_style")
      _line_style = QtPersistent::extract_i(p);
  XML_iterate_close(QtPlot,tag);
}

void QtPlotStyle::query(QWidget* p)
{
  StyleDialog* d = new StyleDialog(p, 
                                   _symbol_size, _symbol_style,
                                   _line_size, _line_style);
  d->exec();

  _symbol_size  = d->symbolSize();
  _symbol_style = d->symbolStyle();
  _line_size    = d->lineSize();
  _line_style   = d->lineStyle();
}

void QtPlotStyle::setPlotType(Ami::DescEntry::Type t)
{
  if (_symbol_size==Initial_symbol_size) {
    switch(t) {
    case Ami::DescEntry::Prof:
    case Ami::DescEntry::Scan:
      _symbol_size = Default_symbol_size;
    default:
      break;
    }
  }
}
