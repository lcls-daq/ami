#include "PnccdCalibrator.hh"

#include "ami/qt/PnccdClient.hh"
#include "ami/qt/ZoomPlot.hh"

#include "ami/event/PnccdCalib.hh"

#include "ami/data/Average.hh"
#include "ami/data/Variance.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/RawFilter.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>


using namespace Ami::Qt;

static Ami::RawFilter _noFilter;

PnccdCalibrator::PnccdCalibrator(PnccdClient* parent) :
  QtPWidget (parent),
  _parent   (parent),
  _ped      (this, _parent->title()+"_pedcal", new Ami::Average),
  _noise    (this, _parent->title()+"_noisecal", new Ami::Variance),
  _acquiring(false)
{
  setWindowTitle("Calibration");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  _acqB  = new QPushButton("Start Acquisition");

  _factor= new QLineEdit("3.0");
  new QDoubleValidator(_factor);

  _saveB = new QPushButton("Save Calibration");
  _saveB->setEnabled(false);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(_acqB);
  { QVBoxLayout* hl = new QVBoxLayout;
    hl->addWidget(new QLabel("CommonModeFactor"));
    hl->addWidget(_factor);
    layout->addLayout(hl); }
  layout->addWidget(_saveB);
  setLayout(layout);

  connect(_acqB , SIGNAL(clicked()), this, SLOT(acquire()));
  connect(_saveB, SIGNAL(clicked()), this, SLOT(writecal()));
}

PnccdCalibrator::~PnccdCalibrator()
{
}

void PnccdCalibrator::save(char*& p) const
{
  XML_insert(p, "QtPWidget", "self", QtPWidget::save(p) );
}

void PnccdCalibrator::load(const char*& p) 
{
  XML_iterate_open(p,tag)
   if (tag.element == "QtPWidget")
      QtPWidget::load(p);
  XML_iterate_close(PnccdCalibrator,tag);
}

void PnccdCalibrator::snapshot(const QString& p) const
{}

void PnccdCalibrator::save_plots(const QString& p) const
{}

void PnccdCalibrator::configure(char*& p, unsigned input, unsigned& output,
                                ChannelDefinition* channels[], int* signatures, unsigned nchannels)
{
  if (_acquiring) {
    _ped  .configure(p,input,output);
    _noise.configure(p,input,output);
  }
}

void PnccdCalibrator::setup_payload(Ami::Cds& cds)
{
  if (_acquiring) {
    _ped  .setup_payload(cds);
    _noise.setup_payload(cds);
  }
}

void PnccdCalibrator::update()
{
  _ped  .update();
  _noise.update();
}

void PnccdCalibrator::acquire()
{
  _acquiring = !_acquiring;
  _ped  .acquire(_acquiring);
  _noise.acquire(_acquiring);

  if (_acquiring) {
    //  Configure the input to generate an uncalibrated raw image
    _fnBox_state = _parent->_fnBox->isChecked();
    _npBox_state = _parent->_npBox->isChecked();
    _parent->_fnBox->setChecked(false);
    _parent->_npBox->setChecked(true);
    _acqB->setText("Stop Acquisition");
    _saveB->setEnabled(false);
  }
  else {
    _parent->_fnBox->setChecked(_fnBox_state);
    _parent->_npBox->setChecked(_npBox_state);
    _acqB->setText("Start Acquisition");
    _saveB->setEnabled(true);
  }
  emit changed();
}

void PnccdCalibrator::writecal()
{
  bool lProd = QString(getenv("HOME")).endsWith("opr");
  PnccdCalib::save_pedestals(_ped  .entry(),
                             _parent->rotation(),
                             lProd);
  PnccdCalib::save_cmth     (_noise.entry(),
                             _parent->rotation(),
                             lProd,
                             _factor->text().toDouble());
  _ped  .hide();
  _noise.hide();
  _parent->_reloadPedestals=true;
  emit changed();
}


PnccdCalibrator::Param::Param(QWidget* parent, QString title, AbsOperator* op) :
  _plot(new ZoomPlot(parent,title,false)),
  _op       (op),
  _signature(0),
  _entry    (0),
  _result   (0)
{
  _plot->hide();
}

PnccdCalibrator::Param::~Param()
{
  delete _op;
  if (_result) delete _result;
}

void PnccdCalibrator::Param::configure(char*& p, unsigned input, unsigned& output)
{
  ConfigureRequest& r = *new(p) ConfigureRequest(ConfigureRequest::Create,
                                                 ConfigureRequest::Discovery,
                                                 input,
                                                 -1,
                                                 _noFilter,
                                                 *_op);
  p += r.size();
  _req.request(r, output, false);
  _signature = r.output();

  _plot->configure(p,_signature,output);
}

void PnccdCalibrator::Param::setup_payload(Ami::Cds& cds)
{
  _entry = static_cast<const EntryImage*>(cds.entry(_signature));
  _plot->setup_payload(cds);
}

void PnccdCalibrator::Param::update()
{
  _plot->update();
}

void PnccdCalibrator::Param::acquire(bool v)
{
  if (v) {
    _plot->show();
  }
  else {
    if (_result) delete _result;
    _result = new EntryImage(_entry->desc());
    _result->setto(*_entry); 
  }
}

void PnccdCalibrator::Param::hide() { _plot->hide(); }

const Ami::EntryImage* PnccdCalibrator::Param::entry()
{
  return _result;
}
