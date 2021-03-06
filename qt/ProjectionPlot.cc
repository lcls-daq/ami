#include "ProjectionPlot.hh"

#include "ami/qt/AxisArray.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/CursorsX.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/Fit.hh"
#include "ami/qt/PeakFit.hh"
#include "ami/qt/PlotFactory.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/WaveformDisplay.hh"
#include "ami/qt/PWidgetManager.hh"
#include "ami/qt/QtUtils.hh"

#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/XYProjection.hh"
#include "ami/data/RPhiProjection.hh"
#include "ami/data/ContourProjection.hh"
#include "ami/data/XYHistogram.hh"
#include "ami/data/TdcPlot.hh"
#include "ami/data/FFT.hh"
#include "ami/data/RawFilter.hh"
#include "ami/data/EntryScalarRange.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QGroupBox>
#include <QtGui/QButtonGroup>
#include <QtGui/QPushButton>

#include "qwt_plot.h"
#include <stdio.h>

using namespace Ami::Qt;

static QColor  color[] = { QColor(0,0,255), QColor(255,0,0), QColor(0,255,0), QColor(255,0,255) };
static QStringList names = QStringList() << QString("ChA") << QString("ChB") << QString("ChC") << QString("ChD");


ProjectionPlot::ProjectionPlot(QWidget*          parent,
			       const QString&    name,
			       unsigned          input_channel,
			       Ami::AbsOperator* proj) :
  QtPWidget(0),
  _name    (name),
  _input   (input_channel),
  _output  (-1),
  _proj    (proj),
  _frame   (new WaveformDisplay),
  _showMask(1),
  _auto_range(0),
  _chrome_changed(false)
{
  //
  //  Ideally, we would set the attribute DeleteOnClose to false and destroy this object
  //  when all of its child plots are closed.
  //
  for(int i=0; i<NCHANNELS; i++)
    _channels[i] = new ChannelDefinition(this, names[i], _channels, i, NCHANNELS,
					 *_frame, color[i], i==0);

  _cursors = new CursorsX(this,_channels,NCHANNELS,*_frame, this);
  _peakfit = new PeakFit (this,_channels,NCHANNELS,*_frame, this);
  _fit     = new Fit     (this,_channels,NCHANNELS,*_frame, this);

  _layout();
  
  PWidgetManager::add(this, _name);
}

ProjectionPlot::ProjectionPlot(QWidget*          parent,
			       const char*&      p) :
  QtPWidget(0),
  _output  (-1),
  _frame   (new WaveformDisplay),
  _auto_range(0),
  _chrome_changed(false)
{
  for(int i=0; i<NCHANNELS; i++)
    _channels[i] = new ChannelDefinition(this, names[i], _channels, i, NCHANNELS,
					 *_frame, color[i], i==0);
	
  _cursors = new CursorsX(this,_channels,NCHANNELS,*_frame, this);
  _peakfit = new PeakFit (this,_channels,NCHANNELS,*_frame, this);
  _fit     = new Fit     (this,_channels,NCHANNELS,*_frame, this);

  load(p);

  _layout();

  PWidgetManager::add(this, _name);
}

ProjectionPlot::~ProjectionPlot()
{
  for(int i=0; i<NCHANNELS; i++)
    delete _channels[i];
  delete _cursors;
  delete _peakfit;
  delete _fit;

  PWidgetManager::remove(this);
  delete _proj;

  delete _showPlotBoxes;
}

void ProjectionPlot::_layout()
{
  setWindowTitle(_name);
  setAttribute(::Qt::WA_DeleteOnClose, true);

  _showPlotBoxes = new QButtonGroup;
  _showPlotBoxes->setExclusive( !_frame->canOverlay() );

  QHBoxLayout* layout = new QHBoxLayout;
  { QVBoxLayout* layout3 = new QVBoxLayout;
    { QGroupBox* chanBox = new QGroupBox("Channels");
      QVBoxLayout* layout1 = new QVBoxLayout;
      QPushButton* chanB[NCHANNELS];
      for(int i=0; i<NCHANNELS; i++) {
	chanB[i] = new QPushButton(names[i]);
	chanB[i]->setPalette(QPalette(color[i]));
	{ QHBoxLayout* layout4 = new QHBoxLayout;
	  QCheckBox* box = new QCheckBox("");
	  _showPlotBoxes->addButton(box);
	  connect(box, SIGNAL(toggled(bool)), _channels[i], SLOT(show_plot(bool)));
	  box->setChecked( _showMask & (1<<i) );
	  layout4->addWidget(box);
	  layout4->addWidget(chanB[i]);
	  layout1->addLayout(layout4);
	  connect(chanB[i], SIGNAL(clicked()), _channels[i], SLOT(front()));
	  connect(_channels[i], SIGNAL(changed()), this, SLOT(update_configuration()));
	  connect(_channels[i], SIGNAL(newplot(bool)), box , SLOT(setChecked(bool))); }
      }
      chanBox->setLayout(layout1);
      layout3->addWidget(chanBox); }
    { QPushButton* cursorsB = new QPushButton("Cursors");
      layout3->addWidget(cursorsB);
      connect(cursorsB, SIGNAL(clicked()), _cursors, SLOT(front())); }
    { QPushButton* peakFitB = new QPushButton("Peak");
      layout3->addWidget(peakFitB);
      connect(peakFitB, SIGNAL(clicked()), _peakfit, SLOT(front())); } 
    { QPushButton* fitB = new QPushButton("Fit");
      layout3->addWidget(fitB);
      connect(fitB, SIGNAL(clicked()), _fit, SLOT(front())); }
    layout3->addStretch();
    layout->addLayout(_chrome_layout=layout3); }
  layout->addWidget(_frame);
  setLayout(layout);

  connect(_cursors, SIGNAL(changed()), this, SLOT(update_configuration()));
  connect(_peakfit, SIGNAL(changed()), this, SLOT(update_configuration()));
  connect(_fit    , SIGNAL(changed()), this, SLOT(update_configuration()));
  connect(_frame  , SIGNAL(set_chrome_visible(bool)), this, SLOT(set_chrome_visible(bool)));

  show();
}

void ProjectionPlot::save(char*& p) const
{
  char* buff = new char[8*1024];

  XML_insert( p, "QtPWidget", "self",
              QtPWidget::save(p) );

  XML_insert( p, "QString", "_name",
              QtPersistent::insert(p,_name) );
  XML_insert( p, "unsigned", "_input", 
              QtPersistent::insert(p,_input) );

  XML_insert( p, "Ami::AbsOperator", "_proj", 
              QtPersistent::insert(p,buff,(char*)_proj->serialize(buff)-buff) );

  for(unsigned i=0; i<NCHANNELS; i++)
    XML_insert( p, "ChannelDefinition", "_channels",
                _channels[i]->save(p) );

  XML_insert( p, "WaveformDisplay", "_frame",
              _frame  ->save(p) );
  XML_insert( p, "CursorsX", "_cursors",
              _cursors->save(p) );
  XML_insert( p, "PeakFit", "_peakfit",
              _peakfit->save(p) );
  XML_insert( p, "Fit", "_fit",
              _fit->save(p) );

  delete[] buff;
}

void ProjectionPlot::load(const char*& p)
{
  _showMask = 0;
  unsigned nchannels=0;

  XML_iterate_open(p,tag)
    if      (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.name == "_name")
      _name  = QtPersistent::extract_s(p);
    else if (tag.name == "_input")
      _input = QtPersistent::extract_i(p);
    else if (tag.name == "_channels") {
      _channels[nchannels]->load(p);
      if (_channels[nchannels]->is_shown())
        _showMask |= 1<<nchannels;
      nchannels++;
    }
    else if (tag.name == "_proj") {
      const char* v = (const char*)QtPersistent::extract_op(p);
      uint32_t type = (AbsOperator::Type)*reinterpret_cast<const uint32_t*>(v);
      v+=2*sizeof(uint32_t); // type and next
      switch(type) {
      case AbsOperator::XYProjection     : _proj = new XYProjection     (v); break;
      case AbsOperator::RPhiProjection   : _proj = new RPhiProjection   (v); break;
      case AbsOperator::ContourProjection: _proj = new ContourProjection(v); break;
      case AbsOperator::XYHistogram      : _proj = new XYHistogram      (v); break;
      case AbsOperator::TdcPlot          : _proj = new TdcPlot          (v); break;
      case AbsOperator::FFT              : _proj = new FFT              (v); break;
      default: _proj=0; printf("Unable to parse projection type %d\n",type); break;
      }
    }
    else if (tag.name == "_frame")
      _frame  ->load(p);
    else if (tag.name == "_cursors")
      _cursors->load(p);
    else if (tag.name == "_peakfit")
      _peakfit->load(p);
    else if (tag.name == "_fit")
      _fit->load(p);
  XML_iterate_close(ProjectionPlot,tag);
}

void ProjectionPlot::save_plots(const QString& p) const
{
  _frame  ->save_plots(p);
  _cursors->save_plots(p+"_cursor");
  _peakfit->save_plots(p+"_peakfit");
  _fit    ->save_plots(p+"_fit");
}

void ProjectionPlot::update_configuration()
{
  emit description_changed();
}

#include "ami/data/Entry.hh"

void ProjectionPlot::setup_payload(Cds& cds)
{
  _auto_range = 0;

  Ami::Entry* entry = cds.entry(_output);
  if (entry && entry->desc().type() == Ami::DescEntry::ScalarRange)
    _auto_range = static_cast<const Ami::EntryScalarRange*>(entry);
  else
    cds.request(*entry, false);  // not shown

  _frame->reset();

  for(unsigned i=0; i<NCHANNELS; i++)
    _channels[i]->setup_payload(cds);

  _cursors->setup_payload(cds);
  _peakfit->setup_payload(cds);
  _fit    ->setup_payload(cds);
}

void ProjectionPlot::configure(char*& p, unsigned input, unsigned& output, ConfigureRequest::Source s)
{
  ConfigureRequest& req = *new (p) ConfigureRequest(ConfigureRequest::Create,
                                                    s,
                                                    input,
                                                    -1,
                                                    RawFilter(),
                                                    *_proj);
  p += req.size();
  _req.request(req, output);
  input = _output = req.output();

  int signatures[NCHANNELS];
  for(int i=0; i<NCHANNELS; i++)
    signatures[i] = -1;

  //
  //  Configure channels which depend upon others
  //
  bool lAdded;
  do {
    lAdded=false;
    for(unsigned i=0; i<NCHANNELS; i++) {
      if (signatures[i]<0) {
	int sig = _channels[i]->configure(p,input,output,
					  _channels,signatures,NCHANNELS,
					  ConfigureRequest::Analysis);
	if (sig >= 0) {
	  signatures[i] = sig;
	  lAdded = true;
	  //	    printf("Added signature %d for channel %d\n",sig,i);
	}
      }
    }
  } while(lAdded);


  _cursors->configure(p,input,output,
		      _channels,signatures,NCHANNELS,
		      ConfigureRequest::Analysis);
  _peakfit->configure(p,input,output,
		      _channels,signatures,NCHANNELS,
		      ConfigureRequest::Analysis);
  _fit    ->configure(p,input,output,
		      _channels,signatures,NCHANNELS,
                      ConfigureRequest::Analysis);
}

void ProjectionPlot::configure(char*& p, unsigned input, unsigned& output,
			       ChannelDefinition* input_channels[], int* input_signatures, unsigned input_nchannels)
{
  printf("ProjectionPlot::configure %p  input %d  output %d\n",p,input,output);

  ConfigureRequest& req = *new (p) ConfigureRequest(ConfigureRequest::Create,
                                                    ConfigureRequest::Analysis,
                                                    input_signatures[_input],
                                                    -1,
                                                    *input_channels[_input]->filter().filter(),
                                                    *_proj);

  p += req.size();
  _req.request(req, output);
  input = _output = req.output();

  int signatures[NCHANNELS];
  for(int i=0; i<NCHANNELS; i++)
    signatures[i] = -1;

  //
  //  Configure channels which depend upon others
  //
  bool lAdded;
  do {
    lAdded=false;
    for(unsigned i=0; i<NCHANNELS; i++) {
      if (signatures[i]<0) {
	int sig = _channels[i]->configure(p,input,output,
					  _channels,signatures,NCHANNELS,
					  ConfigureRequest::Analysis);
	if (sig >= 0) {
	  signatures[i] = sig;
	  lAdded = true;
	  //	    printf("Added signature %d for channel %d\n",sig,i);
	}
      }
    }
  } while(lAdded);


  _cursors->configure(p,input,output,
		      _channels,signatures,NCHANNELS,
		      ConfigureRequest::Analysis);
  _peakfit->configure(p,input,output,
		      _channels,signatures,NCHANNELS,
		      ConfigureRequest::Analysis);
  _fit    ->configure(p,input,output,
		      _channels,signatures,NCHANNELS,
                      ConfigureRequest::Analysis);
}

void ProjectionPlot::update()
{
  _frame  ->update();
  _cursors->update();
  _peakfit->update();
  _fit    ->update();

  if (_auto_range) {
    double v = _auto_range->entries() - double(_auto_range->desc().nsamples());
    if (v >= 0) {
      //  Want only single event distributions?
      _auto_range->result(&_proj->output())->aggregate(false);
      _auto_range = 0;
      emit description_changed();
    }
  }
}

void ProjectionPlot::set_chrome_visible(bool v)
{
  _chrome_changed = true;
  QtUtils::setChildrenVisible(_chrome_layout, v);
  updateGeometry();
  resize(minimumWidth(),minimumHeight());
}

void ProjectionPlot::paintEvent(QPaintEvent* e)
{
  if (_chrome_changed) {
    resize(minimumWidth(),minimumHeight());
    _chrome_changed = false;
  }
  QWidget::paintEvent(e);
}
