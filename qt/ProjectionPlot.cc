#include "ProjectionPlot.hh"

#include "ami/qt/AxisArray.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/CursorsX.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/PeakFit.hh"
#include "ami/qt/PlotFactory.hh"
#include "ami/qt/QtTH1F.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/WaveformDisplay.hh"

#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/XYProjection.hh"
#include "ami/data/RPhiProjection.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QGroupBox>
#include <QtGui/QButtonGroup>
#include <QtGui/QPushButton>

#include "qwt_plot.h"

using namespace Ami::Qt;

static QColor  color[] = { QColor(0,0,255), QColor(255,0,0), QColor(0,255,0), QColor(255,0,255) };
static QStringList names = QStringList() << QString("ChA") << QString("ChB") << QString("ChC") << QString("ChD");

ProjectionPlot::ProjectionPlot(QWidget*          parent,
			       const QString&    name,
			       unsigned          input_channel,
			       Ami::AbsOperator* proj) :
  QtPWidget(parent),
  _name    (name),
  _input   (input_channel),
  _proj    (proj),
  _frame   (new WaveformDisplay)
{
  for(int i=0; i<NCHANNELS; i++)
    _channels[i] = new ChannelDefinition(static_cast<QWidget*>(parent), names[i], names, *_frame, color[i], i==0);

  _cursors = new CursorsX(this,_channels,NCHANNELS,*_frame);
  _peakfit = new PeakFit (this,_channels,NCHANNELS,*_frame);

  _layout();
}

ProjectionPlot::ProjectionPlot(QWidget*          parent,
			       const char*&      p) :
  QtPWidget(parent),
  _frame   (new WaveformDisplay)
{
  for(int i=0; i<NCHANNELS; i++)
    _channels[i] = new ChannelDefinition(static_cast<QWidget*>(parent), names[i], names, *_frame, color[i], i==0);
	
  _cursors = new CursorsX(this,_channels,NCHANNELS,*_frame);
  _peakfit = new PeakFit (this,_channels,NCHANNELS,*_frame);

  load(p);

  _layout();
}

ProjectionPlot::~ProjectionPlot()
{
  delete _proj;
}

void ProjectionPlot::_layout()
{
  setWindowTitle(_name);
  setAttribute(::Qt::WA_DeleteOnClose, true);

  QButtonGroup* showPlotBoxes = new QButtonGroup;
  showPlotBoxes->setExclusive( !_frame->canOverlay() );

  QHBoxLayout* layout = new QHBoxLayout;
  { QVBoxLayout* layout3 = new QVBoxLayout;
    { QGroupBox* chanBox = new QGroupBox("Channels");
      QVBoxLayout* layout1 = new QVBoxLayout;
      QPushButton* chanB[NCHANNELS];
      for(int i=0; i<NCHANNELS; i++) {
	chanB[i] = new QPushButton(names[i]); chanB[i]->setCheckable(true);
	chanB[i]->setPalette(QPalette(color[i]));
	{ QHBoxLayout* layout4 = new QHBoxLayout;
	  QCheckBox* box = new QCheckBox("");
	  showPlotBoxes->addButton(box);
	  connect(box, SIGNAL(toggled(bool)), _channels[i], SLOT(show_plot(bool)));
	  box->setChecked( i==0 );
	  layout4->addWidget(box);
	  layout4->addWidget(chanB[i]);
	  layout1->addLayout(layout4);
	  connect(chanB[i], SIGNAL(clicked(bool)), _channels[i], SLOT(setVisible(bool)));
	  connect(_channels[i], SIGNAL(changed()), this, SLOT(update_configuration()));
	  connect(_channels[i], SIGNAL(newplot(bool)), box , SLOT(setChecked(bool))); }
      }
      chanBox->setLayout(layout1);
      layout3->addWidget(chanBox); }
    { QPushButton* cursorsB = new QPushButton("Cursors");
      cursorsB->setCheckable(true);
      layout3->addWidget(cursorsB);
      connect(cursorsB, SIGNAL(clicked(bool)), _cursors, SLOT(setVisible(bool))); }
    { QPushButton* peakFitB = new QPushButton("Peak");
      peakFitB->setCheckable(true);
      layout3->addWidget(peakFitB);
      connect(peakFitB, SIGNAL(clicked(bool)), _peakfit, SLOT(setVisible(bool))); }
    layout3->addStretch();
    layout->addLayout(layout3); }
  layout->addWidget(_frame);
  setLayout(layout);

  connect(_cursors, SIGNAL(changed()), this, SLOT(update_configuration()));
  connect(_peakfit, SIGNAL(changed()), this, SLOT(update_configuration()));
  show();
}

void ProjectionPlot::save(char*& p) const
{
  QtPWidget::save(p);

  QtPersistent::insert(p,_name);
  QtPersistent::insert(p,_input);
  p = (char*)_proj->serialize(p);

  for(unsigned i=0; i<NCHANNELS; i++) _channels[i]->save(p);

  _cursors->save(p);
  _peakfit->save(p);
}

void ProjectionPlot::load(const char*& p)
{
  QtPWidget::load(p);

  _name  = QtPersistent::extract_s(p);
  _input = QtPersistent::extract_i(p);

  { uint32_t type = (AbsOperator::Type)*reinterpret_cast<const uint32_t*>(p);
    p+=2*sizeof(uint32_t); // type and next
    switch(type) {
    case AbsOperator::XYProjection  : _proj = new XYProjection  (p); break;
    case AbsOperator::RPhiProjection: _proj = new RPhiProjection(p); break;
    default: _proj=0; printf("Unable to parse projection type %d\n",type); break;
    }
  }

  for(unsigned i=0; i<NCHANNELS; i++)    _channels[i]->load(p);

  _cursors->load(p);
  _peakfit->load(p);
}

void ProjectionPlot::update_configuration()
{
  emit description_changed();
}

#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

void ProjectionPlot::setup_payload(Cds& cds)
{
  _frame->reset();

  for(unsigned i=0; i<NCHANNELS; i++)
    _channels[i]->setup_payload(cds);

  _cursors->setup_payload(cds);
  _peakfit->setup_payload(cds);
}

void ProjectionPlot::configure(char*& p, unsigned input, unsigned& output,
			       ChannelDefinition* input_channels[], int* input_signatures, unsigned input_nchannels)
{
  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						  ConfigureRequest::Analysis,
						  input_signatures[_input],
						  input = ++output,
						  *input_channels[_input]->filter().filter(),
						  *_proj);
  p += r.size();

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
}

void ProjectionPlot::update()
{
  _frame  ->update();
  _cursors->update();
  _peakfit->update();
}
