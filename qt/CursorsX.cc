#include "CursorsX.hh"

#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/CursorDefinition.hh"
#include "ami/qt/CursorPlot.hh"
#include "ami/qt/CursorPost.hh"
#include "ami/qt/CursorOverlay.hh"
#include "ami/qt/WaveformDisplay.hh"
#include "ami/qt/Calculator.hh"
#include "ami/qt/PlotFrame.hh"
#include "ami/qt/ScalarPlotDesc.hh"
#include "ami/qt/PostAnalysis.hh"
#include "ami/qt/QtPlotSelector.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/DescCache.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/Expression.hh"
#include "ami/data/BinMath.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QComboBox>
#include <QtGui/QDoubleSpinBox>
#include <QtCore/QRegExp>
#include <QtGui/QRegExpValidator>
#include <QtGui/QMessageBox>

#include <sys/socket.h>
#include <stdio.h>

// works for labels not buttons
//#define bold(t) "<b>" #t "</b>"
#define bold(t) #t

enum { _TH1F, _vT, _vF, _vS };

using namespace Ami::Qt;

//static QChar _integrate   (0x2026);  // ..
static QChar _moment2     (0x2a0e);  // S=
static QChar _moment1     (0x2a0d);  // S-
static QChar _integrate   (0x222b);  // S
static QChar _range       (0x2194);  // left-right arrow
static QChar _exponentiate(0x005E);
static QChar _multiply    (0x00D7);
static QChar _divide      (0x00F7);
static QChar _add         (0x002B);
static QChar _subtract    (0x002D);

CursorsX::CursorsX(QWidget* parent, 
                   ChannelDefinition* channels[], 
                   unsigned nchannels, 
                   WaveformDisplay& frame,
                   QtPWidget* frameParent) :
  QtPWidget (parent),
  _channels (channels),
  _nchannels(nchannels),
  _channel  (0),
  _frame    (frame),
  _frameParent(frameParent),
  _clayout  (new QVBoxLayout),
  _expr     (new QComboBox),
  _list_sem (Semaphore::FULL)
{
  _names << "a" << "b" << "c" << "d" << "f" << "g" << "h" << "i" << "j" << "k";

  _expr->setEditable(false);
  _expr->addItem("1");
  _expr->setMaxCount(10);

  _new_value = new CursorLocation;

  setWindowTitle("CursorsX Plot");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QComboBox* channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    channelBox->addItem(channels[i]->name());

  _scalar_desc = new ScalarPlotDesc(0);

  QPushButton* calcB  = new QPushButton("Enter");
  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* ovlyB  = new QPushButton("Overlay");
  QPushButton* closeB = new QPushButton("Close");
  QPushButton* grabB  = new QPushButton("Grab");
  
  QVBoxLayout* layout = new QVBoxLayout;
  { QGroupBox* channel_box = new QGroupBox("Source Channel");
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Channel"));
    layout1->addWidget(channelBox);
    layout1->addStretch();
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  { QGroupBox* locations_box = new QGroupBox("Define CursorsX");
    locations_box->setToolTip("Define a cursor with a NAME and x-axis LOCATION.");
    QVBoxLayout* layout2 = _clayout;
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addWidget(new QLabel("New Cursor Location"));
      layout1->addWidget(_new_value);
      layout1->addWidget(grabB);
      layout2->addLayout(layout1); }
    locations_box->setLayout(layout2);
    layout->addWidget(locations_box); }
  { QGroupBox* expr_box = new QGroupBox("Expression:");
    expr_box->setToolTip(QString("Expression is a set of cursor names with the operations:\n" \
				 "  A %1 B : Integrate between cursors A and B\n"\
				 "  A %2 B : 1st-moment Integral between cursors A and B\n"\
				 "  A %3 B : 2nd-moment Integral between cursors A and B\n"\
				 "  A %4 B : Count of bins between cursors A and B\n"\
				 "  A %5 B : Exponentiate [value at] A to the power of [value at] B\n "	\
				 "  A %6 B : Multiply [value at] A by [value at] B\n "	\
				 "  A %7 B : Divide \n "	\
				 "  A %8 B : Add \n "		\
				 "  A %9 B : Subtract \n ")
			 .arg(_integrate)
			 .arg(_moment1)
			 .arg(_moment2)
			 .arg(_range)
			 .arg(_exponentiate)
			 .arg(_multiply)
			 .arg(_divide)
			 .arg(_add)
			 .arg(_subtract));
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Expr"));
    layout1->addWidget(_expr);
    layout1->addWidget(calcB);
    expr_box->setLayout(layout1); 
    layout->addWidget(expr_box); }
  { layout->addWidget(_scalar_desc); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(plotB);
    layout1->addWidget(ovlyB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  layout->addStretch();
  setLayout(layout);
    
  connect(channelBox, SIGNAL(activated(int)), this, SLOT(set_channel(int)));
  connect(_new_value, SIGNAL(returnPressed()),this, SLOT(add_cursor()));
  connect(calcB     , SIGNAL(clicked()),      this, SLOT(calc()));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(ovlyB     , SIGNAL(clicked()),      this, SLOT(overlay()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));
  connect(closeB    , SIGNAL(clicked()),      this, SIGNAL(closed()));
  connect(grabB     , SIGNAL(clicked()),      this, SLOT(grab_cursorx()));
  connect(this      , SIGNAL(grabbed()),      this, SLOT(add_cursor()));
  connect(this      , SIGNAL(grabbed()),      this, SLOT(front()));

  _plotB = plotB;
  _ovlyB = ovlyB;
  _scalar_desc->post(this, SLOT(add_post()));
}
  
CursorsX::~CursorsX()
{
  _list_sem.take();
  for(std::list<CursorPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    delete *it;
  }
  _posts.clear();
  _list_sem.give();
}

void CursorsX::save(char*& p) const
{
  XML_insert(p, "QtPWidget", "self", QtPWidget::save(p) );

  _expr_save(p);

  XML_insert(p, "ScalarPlotDesc", "_scalar_desc", _scalar_desc->save(p) );

  for(std::list<CursorDefinition*>::const_iterator it=_cursors.begin(); it!=_cursors.end(); it++) {
    XML_insert(p, "CursorDefinition", "_cursors", (*it)->save(p) );
  }

  for(std::list<CursorPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    XML_insert(p, "CursorPlot", "_plots", (*it)->save(p) );
  }
  for(std::list<CursorPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    XML_insert(p, "CursorPost", "_posts", (*it)->save(p) );
  }
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++) {
    XML_insert(p, "CursorOverlay", "_ovls", (*it)->save(p) );
  }
}

void CursorsX::load(const char*& p)
{
  _list_sem.take();

  for(std::list<CursorDefinition*>::const_iterator it=_cursors.begin(); it!=_cursors.end(); it++) {
    _names.push_back((*it)->name());
    delete *it;
  }
  _cursors.clear();

  for(std::list<CursorPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    disconnect(*it, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
    delete *it;
  }
  _plots.clear();

  for(std::list<CursorPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    delete *it;
  }
  _posts.clear();
  _ovls .clear();

  XML_iterate_open(p,tag)
    if      (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.name == "_expr")
      _expr_load(p);
    else if (tag.name == "_scalar_desc")
      _scalar_desc->load(p);
    else if (tag.name == "_cursors") {
      CursorDefinition* d = new CursorDefinition(p, *this, _frame.plot());
      _cursors.push_back(d);
      _clayout->addWidget(d);
      _names.removeAll(d->name());
      printf("Added cursor %s at %g\n",qPrintable(d->name()), d->location());
    }    
    else if (tag.name == "_plots") {
      CursorPlot* plot = new CursorPlot(this, p);
      _plots.push_back(plot);
      connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
    }
    else if (tag.name == "_posts") {
      CursorPost* post = new CursorPost(p);
      _posts.push_back(post);
    }
    else if (tag.name == "_ovls") {
      CursorOverlay* ovl = new CursorOverlay(*this, p);
      _ovls.push_back(ovl);
    }
  XML_iterate_close(CursorsX,tag);

  _list_sem.give();

  setVisible(false);

  emit changed();
}

void CursorsX::snapshot(const QString& p) const
{
  for(std::list<CursorPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->_snapshot(QString("%1_%2.png").arg(p).arg((*it)->_name));
}

void CursorsX::save_plots(const QString& p) const
{
  for(std::list<CursorPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    QString s = QString("%1_%2.dat").arg(p).arg((*it)->_name);
    FILE* f = fopen(qPrintable(s),"w");
    if (f) {
      (*it)->dump(f);
      fclose(f);
    }
  }
}

void CursorsX::configure(char*& p, unsigned input, unsigned& output,
			 ChannelDefinition* channels[], int* signatures, unsigned nchannels,
			 ConfigureRequest::Source source)
{
  _plotB->setEnabled(!_channels[_channel]->smp_prohibit());
  _ovlyB->setEnabled(!_channels[_channel]->smp_prohibit());

  //  map cursor name to value
  std::map<std::string, double> values;
  for(std::list<CursorDefinition*>::const_iterator it=_cursors.begin(); it!=_cursors.end(); it++)
    values[std::string(qPrintable((*it)->name()))]=(*it)->location();

  _list_sem.take();
  for(std::list<CursorPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    if (!_channels[(*it)->channel()]->smp_prohibit())
      (*it)->configure(p,input,output,channels,signatures,nchannels,_frame.xinfo(),source,values);
  for(std::list<CursorPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++)
    if (!_channels[(*it)->channel()]->smp_prohibit())
      (*it)->configure(p,input,output,channels,signatures,nchannels,_frame.xinfo(),source,values);
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    if (!_channels[(*it)->channel()]->smp_prohibit())
      (*it)->configure(p,input,output,channels,signatures,nchannels,_frame.xinfo(),source,values);
  _list_sem.give();
}

void CursorsX::setup_payload(Cds& cds)
{
  _list_sem.take();
  for(std::list<CursorPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->setup_payload(cds);
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->setup_payload(cds);
  _list_sem.give();
}

void CursorsX::update()
{
  _list_sem.take();
  for(std::list<CursorPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->update();
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->update();
  _list_sem.give();
}

void CursorsX::initialize(const DescEntry& e)
{
}

void CursorsX::set_channel(int c) 
{ 
  _channel=c; 
  _plotB->setEnabled(!_channels[c]->smp_prohibit());
  _ovlyB->setEnabled(!_channels[c]->smp_prohibit());
}

void CursorsX::add_cursor()
{
  if (_names.size()) {
    CursorDefinition* d = new CursorDefinition(_names.takeFirst(),
					       _new_value->value(),
					       *this,
					       _frame.plot());
    _cursors.push_back(d);
    _clayout->addWidget(d);
    connect(d, SIGNAL(changed()), this, SIGNAL(changed()));
  }
  else {
    QMessageBox::critical(this,tr("Add Cursor"),tr("Too many cursors in use"));
  }
}

void CursorsX::remove(CursorDefinition& c)
{
  _names.push_back(c.name());
  _cursors.remove(&c);
  delete &c;
}

void CursorsX::calc()
{
  QStringList variables;
  for(std::list<CursorDefinition*>::const_iterator it=_cursors.begin(); it!=_cursors.end(); it++)
    variables << (*it)->name();

  QStringList ops;
  ops << _exponentiate
      << _multiply
      << _divide
      << _add   
      << _subtract;

  QStringList vops;
  vops << _range
       << _integrate
       << _moment1
       << _moment2;

  Calculator* c = new Calculator(this,
                                 tr("Cursor Math"),"",
 				 variables, vops, ops);
  if (c->exec()==QDialog::Accepted)
    _expr_setText(c->result());

   delete c;
}

QString CursorsX::_translate_expr()
{
  // replace operator symbols with 8-bit char
  QString expr = _expr_text();
  expr.replace(_integrate   ,BinMath::integrate());
  expr.replace(_moment1     ,BinMath::moment1  ());
  expr.replace(_moment2     ,BinMath::moment2  ());
  expr.replace(_range       ,BinMath::range    ());
  expr.replace(_exponentiate,Expression::exponentiate());
  expr.replace(_multiply    ,Expression::multiply());
  expr.replace(_divide      ,Expression::divide());
  expr.replace(_add         ,Expression::add());
  expr.replace(_subtract    ,Expression::subtract());
  return expr;
}

void CursorsX::plot()
{
  if (_scalar_desc->postAnalysis()) {
    QString     qtitle = _add_post();
    DescEntry*  entry  = _scalar_desc->desc(qPrintable(qtitle));
    PostAnalysis::instance()->plot(qtitle,entry,_posts.back());
  }
  else {
    QString expr = _translate_expr();
    DescEntry* desc = _scalar_desc->desc(qPrintable(_expr_text()));
    CursorPlot* plot = new CursorPlot(this,
                                      _expr_text(),
                                      _channel,
                                      new BinMath(*desc,_scalar_desc->expr(expr)));
    delete desc;

    _list_sem.take();
    _plots.push_back(plot);
    _list_sem.give();

    connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
    connect(plot, SIGNAL(changed()), this, SIGNAL(changed()));
  }
  emit changed();
}

void CursorsX::overlay()
{
  if (_scalar_desc->postAnalysis()) {
    QString     qtitle = _add_post();
    DescEntry*  entry  = _scalar_desc->desc(qPrintable(qtitle));
    new QtPlotSelector(*this, *PostAnalysis::instance(), entry, _posts.back());
  }
  else {
    DescEntry* desc = _scalar_desc->desc(qPrintable(_expr_text()));
    new QtPlotSelector(*this, *this, desc);
  }
}

void CursorsX::add_overlay(DescEntry* desc, QtPlot* plot, SharedData*)
{
  QString expr = _translate_expr();
  CursorOverlay* ovl = new CursorOverlay(*this, 
                                         *plot,
                                         _channel, 
                                         new BinMath(*desc,_scalar_desc->expr(expr)));
  delete desc;
  _list_sem.take();
  _ovls.push_back(ovl);
  _list_sem.give();

  connect(ovl, SIGNAL(changed()), this, SIGNAL(changed()));
  emit changed();
}

void CursorsX::remove_overlay(QtOverlay* obj)
{
  CursorOverlay* ovl = static_cast<CursorOverlay*>(obj);
  _list_sem.take();
  _ovls.remove(ovl);
  _list_sem.give();
}

void    CursorsX::add_post() 
{
  _add_post();
  _posts.back()->signup();
}

QString CursorsX::_add_post()
{
  QString expr = _translate_expr();

  QString qtitle = FeatureRegistry::instance(Ami::PostAnalysis).validate_name(_scalar_desc->qtitle());

  Ami::DescCache* desc = new Ami::DescCache(qPrintable(qtitle),
                                            qPrintable(qtitle),
                                            Ami::PostAnalysis);

  CursorPost* post = new CursorPost(_channel,
				    new BinMath(*desc,_scalar_desc->expr(expr)),
                                    this);
  _list_sem.take();
  _posts.push_back(post);
  _list_sem.give();

  delete desc;

  emit changed();

  FeatureRegistry::instance(Ami::PostAnalysis).share(qtitle,post);

  return qtitle;
}

void CursorsX::hide_cursors()
{
}

void CursorsX::remove_plot(QObject* obj)
{
  CursorPlot* plot = static_cast<CursorPlot*>(obj);
  _list_sem.take();
  _plots.remove(plot);
  _list_sem.give();

  disconnect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  delete obj;
}

void CursorsX::grab_cursorx() 
{
  _frame.plot()->set_cursor_input(this); 
  if (_frameParent)
    _frameParent->front();
}

void CursorsX::mousePressEvent(double x, double y)
{
  _frame.plot()->set_cursor_input(0);
  _new_value->setText(QString::number(x));
  emit grabbed();
}

void CursorsX::mouseMoveEvent   (double,double) {}
void CursorsX::mouseReleaseEvent(double,double) {}

#if 0

QString CursorsX::_expr_text() const { return _expr->text(); }

void CursorsX::_expr_setText(const QString& t) { _expr->setText(t); }

void CursorsX::_expr_save() const
{
  XML_insert(p, "QLineEdit", "_expr", QtPersistent::insert(p,_expr ->text()) );
}

void CursorsX::_expr_load(const char*& p)
{
  _expr->setText(QtPersistent::extract_s(p));
}

#else

static void _save_combobox(char*& p, QComboBox* b)
{
  XML_insert(p, "int", "index", QtPersistent::insert(p, b->currentIndex()));
  for(int i=0; i<b->count(); i++)
    XML_insert(p, "QString", "item", QtPersistent::insert(p, b->itemText(i)));
}

static void _load_combobox(const char*& p, QComboBox* b)
{
  int index = 0;
  XML_iterate_open(p,tag)
  if      (tag.element == "int")
    index = QtPersistent::extract_i(p);
  else if (tag.element == "QString")
    b->insertItem(b->count(), QtPersistent::extract_s(p));
  XML_iterate_close(QComboBox,tag);
  b->setCurrentIndex(index);
}

void CursorsX::_expr_save(char*& p) const
{
  XML_insert(p, "QComboBox", "_expr", _save_combobox(p,_expr) );
}

void CursorsX::_expr_load(const char*& p)
{
  _load_combobox(p, _expr);
}

QString CursorsX::_expr_text() const { return _expr->currentText(); }
void CursorsX::_expr_setText(const QString& t) 
{
  _expr->insertItem(0, t); 
  _expr->setCurrentIndex(0);
}

#endif

void CursorsX::remove_cursor_post(CursorPost* post)
{
  _list_sem.take();
  _posts.remove(post);
  _list_sem.give();

  emit changed();
}
