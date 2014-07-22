#include "ChannelDefinition.hh"

#include "ami/qt/Display.hh"
#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/ChannelMath.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/Transform.hh"
#include "ami/qt/PlotFactory.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/FeatureCalculator.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/MaskDisplay.hh"
#include "ami/qt/QtBase.hh"
#include "ami/qt/SMPRegistry.hh"
#include "ami/qt/SMPWarning.hh"

#include "ami/data/AbsOperator.hh"
#include "ami/data/Reference.hh"
#include "ami/data/Average.hh"
#include "ami/data/Variance.hh"
#include "ami/data/Single.hh"
#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/EntryRefOp.hh"
#include "ami/data/MaskImage.hh"
#include "ami/data/FIR.hh"
#include "ami/data/SelfExpression.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QButtonGroup>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QIntValidator>
#include <QtGui/QComboBox>
#include <QtGui/QCheckBox>

#include <sys/stat.h>

#define bold(t) #t

static inline int avgRound(int n, int d)
{
  return n<0 ? -1 : (n+d-1)/d;
}

using namespace Ami::Qt;

enum { _None, _Single, _Average, _Variance, _Math, _Reference };
enum { REFERENCE_BUFFER_SIZE = 0x1000 };

static const unsigned NOT_INIT = (unsigned)-1;

ChannelDefinition::ChannelDefinition(QWidget* parent,
				     const QString& name, 
				     ChannelDefinition** chs,
				     unsigned            ich,
				     unsigned            nch,
				     Display& frame, 
				     const QColor& color, 
				     bool init,
                                     QStringList refnames) :
  QtPWidget        (parent),
  _name            (name),
  _frame           (frame),
  _color           (color),
  _filter          (new Filter   (this,name)),
  _operator        (0),
  _transform       (new Transform(this,QString("%1 : Y Transform").arg(name),"y")),
  _math            (new ChannelMath(chs,ich,nch)),
  _interval        (new QLineEdit),
  _intervalq       (new QLabel),
  _output_signature(NOT_INIT),
  _changed         (false),
  _show            (false),
  _plot            (0),
  _scale           (new QLineEdit),
  _operator_is_ref (false),
  _configured_ref  (false),
  _mask_file_mtime (0),
  _mask_display    (new MaskDisplay),
  _smp_warning     (new SMPWarning),
  _smp_prohibit    (true)
{
  setWindowTitle(_name);
  setAttribute(::Qt::WA_DeleteOnClose, false);

  _plot_grp = new QButtonGroup;
  QRadioButton* noneB     = new QRadioButton(bold(None));  
  QRadioButton* singleB   = new QRadioButton(bold(Single));  
  QRadioButton* averageB  = new QRadioButton(bold(Averaged));
  QRadioButton* varianceB = new QRadioButton(bold(Variance));
  QRadioButton* mathB     = new QRadioButton(bold(Math));
  QRadioButton* refB      = new QRadioButton(bold(Reference));

  _plot_grp->addButton(noneB    , _None); 
  _plot_grp->addButton(singleB  , _Single); 
  _plot_grp->addButton(averageB , _Average);
  _plot_grp->addButton(varianceB, _Variance);
  _plot_grp->addButton(mathB    , _Math);
  _plot_grp->addButton(refB     , _Reference);
  refB->setEnabled(false);

  new QIntValidator(_interval);

  QPushButton* loadB = new QPushButton("Load");

  QPushButton* filterB  = new QPushButton("Filter");
  QPushButton* yfuncB   = new QPushButton("Y Transform");
  QPushButton* applyB   = new QPushButton("OK");
  QPushButton* closeB   = new QPushButton("Close");
  QPushButton* scaleB   = new QPushButton("Enter");

  _maskB = new QCheckBox  ("Apply Mask");
  QPushButton* mloadB = new QPushButton("Load");
  QPushButton* meditB = new QPushButton("Edit");

  _firB  = new QCheckBox  ("Apply FIR Filter");
  QPushButton* floadB = new QPushButton("Load");

  QVBoxLayout* l = new QVBoxLayout;
  { if (!refnames.empty()) {
      _refBox = new QComboBox;
      _refBox->addItems(refnames);
      _refBox->setCurrentIndex(ich);
      l->addWidget(_refBox);
    }
    else {
      _refBox = 0;
    }
    QGroupBox* plot_box = new QGroupBox("Plot");
    QVBoxLayout* layout = new QVBoxLayout;
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addWidget(new QLabel("Normalize to"));
      layout1->addWidget(_scale);
      layout1->addWidget(scaleB);
      layout->addLayout(layout1); }
    layout->addWidget(noneB);
    layout->addWidget(singleB);
    { QGridLayout* layout1 = new QGridLayout;
      layout1->addWidget(averageB ,0,0);
      layout1->addWidget(varianceB,1,0);
      layout1->addWidget(new QLabel("Events"),0,1,2,1);
      layout1->addWidget(_interval ,0,2,2,1);
      layout1->addWidget(_intervalq,0,3,2,1);
      layout1->addWidget(_smp_warning,0,4,2,1);
      layout->addLayout(layout1); }
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addWidget(mathB);
      layout1->addStretch();
      layout1->addWidget(_math);
      layout->addLayout(layout1); }
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addWidget(refB);
      layout1->addWidget(loadB);
      layout->addLayout(layout1); }
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addWidget(_maskB);
      layout1->addWidget(mloadB);
      layout1->addWidget(meditB);
      layout->addLayout(layout1); }
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addWidget(_firB);
      layout1->addWidget(floadB);
      layout->addLayout(layout1); }
    plot_box->setLayout(layout);
    l->addWidget(plot_box); }
  { QHBoxLayout* layout = new QHBoxLayout;
    layout->addStretch();
    layout->addWidget(filterB);
    layout->addWidget(yfuncB);
    layout->addStretch();
    l->addLayout(layout); }
  { QHBoxLayout* layout = new QHBoxLayout;
    layout->addStretch();
    layout->addWidget(applyB);
    layout->addWidget(closeB);
    layout->addStretch(); 
    l->addLayout(layout); }

  if (dynamic_cast<ImageDisplay*>(&frame)!=0) {
    _firB ->hide();
    floadB->hide();
  }
  else {
    _maskB->hide();
    mloadB->hide();
    meditB->hide();
  }

  setLayout(l);

  connect(this    , SIGNAL(reference_loaded(bool)), refB, SLOT(setEnabled(bool)));
  
  connect(loadB   , SIGNAL(clicked()), this, SLOT(load_reference()));
  connect(filterB , SIGNAL(clicked()), _filter, SLOT(front()));
  connect(yfuncB  , SIGNAL(clicked()), _transform , SLOT(front()));
  connect(applyB  , SIGNAL(clicked()), this, SLOT(apply()));
  connect(_filter , SIGNAL(changed()), this, SLOT(apply()));
  connect(closeB  , SIGNAL(clicked()), this, SLOT(hide()));
  connect(scaleB  , SIGNAL(clicked()), this, SLOT(set_scale()));
  connect(mloadB  , SIGNAL(clicked()), this, SLOT(load_mask()));
  connect(meditB  , SIGNAL(clicked()), this, SLOT(edit_mask()));
  connect(floadB  , SIGNAL(clicked()), this, SLOT(load_fir()));
  connect(_interval, SIGNAL(editingFinished()), this, SLOT(update_interval()));

  connect(&SMPRegistry::instance(), SIGNAL(changed()), this, SLOT(update_interval()));
  connect(_plot_grp, SIGNAL(buttonClicked(int)), this, SLOT(change_agg(int)));

  noneB  ->setChecked(!init);
  singleB->setChecked(init);
  apply();
  show_plot(init);
  update_interval();
  (_meditB = meditB)->setEnabled(false);
}
	  
ChannelDefinition::~ChannelDefinition()
{
}

void ChannelDefinition::save(char*& p) const
{
  XML_insert( p, "QtPWidget", "self", QtPWidget::save(p) );

  XML_insert( p, "QButtonGroup", "_plot_grp", QtPersistent::insert(p,_plot_grp->checkedId()) );
  XML_insert( p, "QLineEdit"   , "_interval", QtPersistent::insert(p,_interval->text()) );
  XML_insert( p, "ChannelMath" , "_math"    , QtPersistent::insert(p,_math->expr()) );
  XML_insert( p, "QString"     , "_ref_file", QtPersistent::insert(p,_plot_grp->button(_Reference)->isEnabled() ? _ref_file : QString("")) );
  XML_insert( p, "bool"        , "_show"    , QtPersistent::insert(p,_show) );
  XML_insert( p, "QComboBox"   , "_refBox"  , QtPersistent::insert(p,_refBox ? _refBox->currentIndex() : -1) );
  XML_insert( p, "Filter"      , "_filter"  , _filter   ->save(p) );
  XML_insert( p, "Transform"   , "_transform",_transform->save(p) );
  XML_insert( p, "QLineEdit"   , "_scale"   , QtPersistent::insert(p,_scale->text()) );
  XML_insert( p, "QCheckBox"   , "_maskB"   , QtPersistent::insert(p, _maskB->checkState()==::Qt::Checked) );
  XML_insert( p, "QString"     , "_mask_file", QtPersistent::insert(p, _mask_file) );
  XML_insert( p, "QCheckBox"   , "_firB"     , QtPersistent::insert(p, _firB->checkState()==::Qt::Checked) );
  XML_insert( p, "QString"     , "_fir_file" , QtPersistent::insert(p, _fir_file) );
}

void ChannelDefinition::load(const char*& p)
{
  int  id   = 0;
  bool show = false;

  XML_iterate_open(p,tag)
    if      (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.name == "_plot_grp")
      id = QtPersistent::extract_i(p);
    else if (tag.name == "_interval")
      _interval->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_math")
      _math->expr(QtPersistent::extract_s(p));
    else if (tag.name == "_ref_file") {
      QString rfile = QtPersistent::extract_s(p);
      if (!rfile.isEmpty()) {
        _ref_file = rfile;
        _plot_grp->button(_Reference)->setEnabled(true);
      }
      else
        _plot_grp->button(_Reference)->setEnabled(false);
    }
    else if (tag.name == "_show")
      show = QtPersistent::extract_b(p);
    else if (tag.name == "_refBox") {
      int index = QtPersistent::extract_i(p);
      if (_refBox && index >= 0)
        _refBox->setCurrentIndex(index);
    }
    else if (tag.name == "_filter")
      _filter   ->load(p);
    else if (tag.name == "_transform")
      _transform->load(p);
    else if (tag.name == "_scale")
      _scale    ->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_maskB")
      _maskB    ->setChecked(QtPersistent::extract_b(p) ? ::Qt::Checked : ::Qt::Unchecked);
    else if (tag.name == "_mask_file")
      _mask_file = QtPersistent::extract_s(p);
    else if (tag.name == "_firB")
      _firB     ->setChecked(QtPersistent::extract_b(p) ? ::Qt::Checked : ::Qt::Unchecked);
    else if (tag.name == "_fir_file")
      _fir_file = QtPersistent::extract_s(p);
  XML_iterate_close(ChannelDefinition,tag);

  _plot_grp->button(id)->setChecked(true);

  update_interval();

  show_plot(show);

  apply();

  emit show_plot_changed(show);
}

void ChannelDefinition::load_reference()
{
  QString file = Path::loadReferenceFile(this,_name);

  if (file.isNull()) {
    printf("load_reference file is null\n");
    return;
  }
  
  _ref_file = file;
  _plot_grp->button(_Reference)->setEnabled(true);
  _plot_grp->button(_Reference)->setChecked(true);
  apply();
}

void ChannelDefinition::load_mask()
{
  QString ref_dir(Path::base());
  QString file = QFileDialog::getOpenFileName(this,
                                              "Mask File",
                                              ref_dir, "*.msk;;*.dat");
  
  _mask_file = file;
  _maskB->setChecked(true);
}

void ChannelDefinition::edit_mask()
{
  _mask_display->setup(_plot,_mask_file);
  _mask_display->front();
}

void ChannelDefinition::load_fir()
{
  QString ref_dir(Path::base());
  QString file = QFileDialog::getOpenFileName(this,
                                              "Finite Impulse Response",
                                              ref_dir, "*.fir");
  
  if (file.isNull())
    ;
  else
    _fir_file = file;
}

void ChannelDefinition::update_interval()
{
  unsigned nproc = SMPRegistry::instance().nservers();
  int n = _interval->text().toInt();
  int m = nproc*avgRound(n,nproc);
  if (n>1 && m!=n)
    _intervalq->setText(QString("(%1)").arg(QString::number(m)));
  else
    _intervalq->clear();

  _update_agg();
}

void ChannelDefinition::show_plot(bool s) 
{
  //  This should be a slot on the display (QwtPlot)
  if (s != _show && _plot!=0) {
    if (s) _frame.show(_plot);
    else   _frame.hide(_plot);
  }
  _show = s;
}

/**
 **  Prepend operator with channel selection
 */
void ChannelDefinition::apply()
{
  // Form _operator
  if (_operator) delete _operator;

  const char* scale = qPrintable(_scale->text());
  unsigned nproc = SMPRegistry::instance().nservers();

  switch(_mode = _plot_grp->checkedId()) {
  case _Single:
    _operator = new Single(scale);
    _operator_is_ref  = false;
    break;
  case _Average     : 
    _operator = new Average  (avgRound(_interval->text().toInt(),nproc),scale);
    _operator_is_ref  = false;
    break;
  case _Variance    : 
    _operator = new Variance (avgRound(_interval->text().toInt(),nproc),scale);
    _operator_is_ref  = false;
    break;
  case _Reference:
    _operator = new Reference(qPrintable(_ref_file));
    _operator_is_ref  = true;
    break;
  case _Math:
  default:
    //  Don't request anything from the server?
    _operator = 0;
    _operator_is_ref  = false;
    break;
  }

  if (_maskB->checkState()==::Qt::Checked && _operator) {
    AbsOperator* op = new MaskImage(qPrintable(_mask_file));
    op->next(_operator);
    _operator = op;
  }

  if (_firB->checkState()==::Qt::Checked && _operator) {
    AbsOperator* op = new FIR(qPrintable(_fir_file));
    op->next(_operator);
    _operator = op;
  }

  if (_refBox) {
    switch(_mode) {
    case _Single:
    case _Average:
    case _Variance:
      { AbsOperator* op = new EntryRefOp(_refBox->currentIndex());
        op->next(_operator);
        _operator = op; }
      break;
    default:
      break;
    }
  }

  _changed = true;
  emit changed();

  emit newplot(_mode!=_None);
}

int ChannelDefinition::configure(char*& p, unsigned input, unsigned& output,
				 ChannelDefinition* channels[], int* signatures, int nchannels,
				 ConfigureRequest::Source source)
{
  ConfigureRequest* r = 0;

  //  if (_mode==_None) 
  //    ;
  //  else if (_mode==_Math) {
  if (_mode==_Math) {
    if (!_math->resolve(channels, signatures, nchannels, *_filter->filter())) {
      printf("ChannelDefinition::_math resolve failed\n");
      return -1;
    }

    if (_refBox) {
      _operator = new EntryRefOp(_refBox->currentIndex());
      _operator->next(&_math->op());
      r = new (p) ConfigureRequest(ConfigureRequest::Create,
                                   source,
                                   input,
                                   -1,
                                   _math->filter(),
                                   *_operator);
      _operator->next(0);
    }
    else {
      r = new (p) ConfigureRequest(ConfigureRequest::Create,
                                   source,
                                   input,
                                   -1,
                                   _math->filter(),
                                   _math->op());
    }
    _configured_ref = false;
  }
  else if (_operator) {
    r = new (p) ConfigureRequest(ConfigureRequest::Create,
                                 source,
                                 input,
                                 -1,
                                 *_filter->filter(),
                                 *_operator);
    _configured_ref = _operator_is_ref;
  }

  if (r) {
    bool force=false;
    struct ::stat64 s;
    if (!_mask_file.isNull() &&
	::stat64(qPrintable(_mask_file),&s)==0 &&
	s.st_mtime != _mask_file_mtime) {
      force=true;
      printf("CD::configure mask modified at %s [%s]\n",
	     ctime(&s.st_mtime), ctime(&_mask_file_mtime));
      _mask_file_mtime = s.st_mtime;
    }
    p += r->size();
    _req.request(*r, output, force);
    return _output_signature = r->output();
  }
  else
    return -1;
}


Ami::AbsTransform& ChannelDefinition::transform()
{
  return *_transform;
}

void ChannelDefinition::setup_payload(Cds& cds, bool vis)
{
  Entry* entry = cds.entry(_output_signature);
  if (entry) {
    cds.request(*entry, _show && vis);
    _plot=PlotFactory::plot(_name,*entry,_frame.xtransform(),transform(),_color,cds.lock());
    if (_plot) {
      _frame.add(_plot, _show);
      _meditB->setEnabled(true);
    }
    else {
      _meditB->setEnabled(false);
    }
    /*
     * OK, a little bit of weirdness here.  If we read in .dat files as references,
     * they look different than the other waveforms and screw everything up.  But
     * if we just ignore the non-zero start points, we screw up the projection
     * waveforms.  Therefore, we keep track if we are a Reference, and if this
     * is the response to configuring a Reference, we ignore it here.
     *
     * If someone only displays unchanging References in a window, they deserve what
     * they are going to get!
     */
    if (_show && !_configured_ref) 
      _frame.prototype(&entry->desc());
  }
  else if (_output_signature!=NOT_INIT)
    printf("%s output_signature %d not found\n",qPrintable(_name),_output_signature);
}

void ChannelDefinition::set_scale()
{
  QStringList self_vars, self_help;
  self_vars << Ami::SelfExpression::self_sum();
  self_help << "The Integral";
  
  FeatureCalculator* c = new FeatureCalculator(this,"%1 : Scale", FeatureRegistry::instance(Ami::PreAnalysis),
                                               self_vars, self_help);
  if (c->exec()==QDialog::Accepted) {
    _scale->setText(c->result());
  }
  delete c;
}

unsigned  ChannelDefinition::output_signature() const { return _output_signature; }

bool      ChannelDefinition::smp_prohibit     () const { return _smp_prohibit; }

void ChannelDefinition::change_agg(int id) { _update_agg(); }

void ChannelDefinition::_update_agg()
{
  int id = _plot_grp->checkedId();
  
  bool ldist = (id == _Average ||
		id == _Variance) &&
    (SMPRegistry::instance().nservers())>1;

  _smp_warning->setEnabled(ldist);

  if (ldist != _smp_prohibit) {
    _smp_prohibit = ldist;
    emit agg_changed();
  }
}
