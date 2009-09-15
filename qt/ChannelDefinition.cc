#include "ChannelDefinition.hh"

#include "ami/qt/Display.hh"
#include "ami/qt/ChannelMath.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/Transform.hh"
#include "ami/qt/PlotFactory.hh"
#include "ami/qt/Path.hh"

#include "ami/data/AbsOperator.hh"
#include "ami/data/Reference.hh"
#include "ami/data/Average.hh"
#include "ami/data/Single.hh"
#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryFactory.hh"

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


#define bold(t) #t

using namespace Ami::Qt;

enum { _None, _Single, _Average, _Math, _Reference };
enum { REFERENCE_BUFFER_SIZE = 0x1000 };

ChannelDefinition::ChannelDefinition(const QString& name, 
				     Display& frame, 
				     const QColor& color, 
				     bool init) :
  QWidget          (0),
  _name            (name),
  _frame           (frame),
  _color           (color),
  _filter          (new Filter   (name)),
  _operator        (0),
  _transform       (new Transform(QString("%1 : Y Transform").arg(name),"y")),
  _math            (new ChannelMath),
  _interval        (new QLineEdit),
  _output_signature(-1UL),
  _changed         (false)
{
  setWindowTitle(_name);
  setAttribute(::Qt::WA_DeleteOnClose, false);

  _plot_grp = new QButtonGroup;
  QRadioButton* noneB    = new QRadioButton(bold(None));  
  QRadioButton* singleB  = new QRadioButton(bold(Single));  
  QRadioButton* averageB = new QRadioButton(bold(Averaged));
  QRadioButton* mathB    = new QRadioButton(bold(Math));
  QRadioButton* refB     = new QRadioButton(bold(Reference));
  _plot_grp->addButton(noneB   , _None); 
  _plot_grp->addButton(singleB , _Single); 
  _plot_grp->addButton(averageB, _Average);
  _plot_grp->addButton(mathB   , _Math);
  _plot_grp->addButton(refB    , _Reference);
  refB->setEnabled(false);

  new QIntValidator(_interval);

  QPushButton* loadB = new QPushButton("Load");

  QPushButton* filterB  = new QPushButton("Filter");
  QPushButton* yfuncB   = new QPushButton("Y Transform");
  QPushButton* displayB = new QPushButton("Display");
  QPushButton* applyB   = new QPushButton("OK");
  QPushButton* closeB   = new QPushButton("Close");
  displayB->setCheckable(true);  // toggles

  QVBoxLayout* l = new QVBoxLayout;
  { QGroupBox* plot_box = new QGroupBox("Plot");
    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(noneB);
    layout->addWidget(singleB);
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addWidget(averageB);
      layout1->addStretch();
      layout1->addWidget(new QLabel("Events"));
      layout1->addWidget(_interval);
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
    plot_box->setLayout(layout);
    l->addWidget(plot_box); }
  { QHBoxLayout* layout = new QHBoxLayout;
    layout->addStretch();
    layout->addWidget(displayB);
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
  setLayout(l);

  connect(this    , SIGNAL(reference_loaded(bool)), refB, SLOT(setEnabled(bool)));

  connect(loadB   , SIGNAL(clicked()), this, SLOT(load_reference()));
  connect(filterB , SIGNAL(clicked()), _filter, SLOT(show()));
  connect(yfuncB  , SIGNAL(clicked()), _transform , SLOT(show()));
  connect(displayB, SIGNAL(clicked(bool)), this, SLOT(show_plot(bool)));
  connect(applyB  , SIGNAL(clicked()), this, SLOT(apply()));
  connect(closeB  , SIGNAL(clicked()), this, SLOT(hide()));

  noneB  ->setChecked(!init);
  singleB->setChecked(init);
  apply();
  show_plot(init);
}
	  
ChannelDefinition::~ChannelDefinition()
{
}

void ChannelDefinition::load_reference()
{
  QString file = Path::loadReferenceFile(_name);

  if (file.isNull()) {
    printf("load_reference file is null\n");
    return;
  }
  
  _ref_file = file;
  _plot_grp->button(_Reference)->setEnabled(true);
  _plot_grp->button(_Reference)->setChecked(true);
  apply();
}
	  
void ChannelDefinition::show_plot(bool s) 
{
  //  This should be a slot on the display (QwtPlot)
}

void ChannelDefinition::apply()
{
  // Form _operator
  if (_operator) delete _operator;

  switch(_mode = _plot_grp->checkedId()) {
  case _Single:
    _operator = new Single ; break;
  case _Average     : 
    _operator = new Average(_interval->text().toInt()); break;
  case _Reference:
    _operator = new Reference(qPrintable(_ref_file)); break;
  case _Math:
  default:
    //  Don't request anything from the server?
    _operator = 0;
    break;
  }

  _changed = true;
  emit changed();
}

int ChannelDefinition::configure(char*& p, unsigned input, unsigned& output,
				 ChannelDefinition* channels[], int* signatures, int nchannels)
{
  //  if (_mode==_None) 
  //    ;
  //  else if (_mode==_Math) {
  if (_mode==_Math) {
    if (!_math->resolve(channels, signatures, nchannels, *_filter->filter()))
      return -1;

    _output_signature = ++output;
    ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						    input,
						    _output_signature,
						    _math->filter(),
						    _math->op());
    p += r.size();
    return _output_signature;
  }
  else if (_operator) {
    printf("mode %d  op %p  type %d\n",_mode,_operator,_operator->type());
    _output_signature = ++output;
//     if (_changed) {
//       _changed=false;
//       _output_signature = ++output;
//     }
    ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						    input,
						    _output_signature,
						    *_filter->filter(),
						    *_operator);
    p += r.size();
    return _output_signature;
  }
  return -1;
}


Ami::AbsTransform& ChannelDefinition::transform()
{
  return *_transform;
}

void ChannelDefinition::setup_payload(Cds& cds)
{
  Entry* entry = cds.entry(_output_signature);
  if (entry)
    _frame.add(PlotFactory::plot(_name,*entry,_frame.xtransform(),transform(),_color));
  else if (_output_signature>=0)
    printf("%s output_signature %d not found\n",qPrintable(_name),_output_signature);
}
