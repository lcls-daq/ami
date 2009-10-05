#include "ImageRPhiProjection.hh"

#include "ami/qt/AnnulusCursors.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/ProjectionPlot.hh"
#include "ami/qt/Display.hh"
#include "ami/qt/ImageFrame.hh"

#include "ami/data/DescTH1F.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/Entry.hh"
#include "ami/data/RPhiProjection.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QButtonGroup>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QMessageBox>

#include <sys/socket.h>

using namespace Ami::Qt;

static const QChar RHO(0x03c1);
static const QChar PHI(0x03c6);

ImageRPhiProjection::ImageRPhiProjection(QWidget*           parent,
					 ChannelDefinition* channels[],
					 unsigned           nchannels, 
					 ImageFrame&        frame) :
  QtPWidget (parent),
  _channels (channels),
  _nchannels(nchannels),
  _channel  (0),
  _frame    (frame),
  _title    (new QLineEdit("Projection")),
  _annulus  (new AnnulusCursors(frame))
{
  setWindowTitle("Image Projection");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QComboBox* channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    channelBox->addItem(channels[i]->name());

  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* closeB = new QPushButton("Close");

  QRadioButton* raxisB = new QRadioButton(RHO);
  QRadioButton* faxisB = new QRadioButton(PHI);
  _axis = new QButtonGroup;
  _axis->addButton(raxisB,0);
  _axis->addButton(faxisB,1);
  raxisB->setChecked(true);

  QRadioButton* sumB  = new QRadioButton("sum");
  QRadioButton* meanB = new QRadioButton("mean");
  _norm = new QButtonGroup;
  _norm->addButton(sumB ,0);
  _norm->addButton(meanB,1);
  sumB->setChecked(true);

  QVBoxLayout* layout = new QVBoxLayout;
  { QGroupBox* channel_box = new QGroupBox("Source Channel");
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Channel"));
    layout1->addWidget(channelBox);
    layout1->addStretch();
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  { QGroupBox* boundary_box = new QGroupBox("Region of Interest");
    QVBoxLayout* layout1 = new QVBoxLayout;
    layout1->addWidget(_annulus);
    boundary_box->setLayout(layout1);
    layout->addWidget(boundary_box); }
  { QGroupBox* plot_box = new QGroupBox("Plot");
    QVBoxLayout* layout1 = new QVBoxLayout;
    { QHBoxLayout* layout2 = new QHBoxLayout;
      layout2->addWidget(new QLabel("Title"));
      layout2->addWidget(_title);
      layout1->addLayout(layout2); }
    { QHBoxLayout* layout2 = new QHBoxLayout;
      layout2->addWidget(plotB);
      layout2->addWidget(new QLabel("Project"));
      { QVBoxLayout* layout3 = new QVBoxLayout;
	layout3->addWidget(sumB);
	layout3->addWidget(meanB);
	layout2->addLayout(layout3); }
      layout2->addWidget(new QLabel("onto"));
      { QVBoxLayout* layout3 = new QVBoxLayout;
	layout3->addWidget(raxisB);
	layout3->addWidget(faxisB);
	layout2->addLayout(layout3); }
      layout2->addWidget(new QLabel("axis"));
      layout2->addStretch();
      layout1->addLayout(layout2); }
    plot_box->setLayout(layout1); 
    layout->addWidget(plot_box); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(plotB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);
    
  connect(channelBox, SIGNAL(activated(int)), this, SLOT(set_channel(int)));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));
}
  
ImageRPhiProjection::~ImageRPhiProjection()
{
}

void ImageRPhiProjection::save(char*& p) const
{
  QtPWidget::save(p);
}

void ImageRPhiProjection::load(const char*& p)
{
  QtPWidget::load(p);
}

void ImageRPhiProjection::setVisible(bool v)
{
  if (v)  _frame.add_marker(*_annulus);
  else    _frame.remove_marker(*_annulus);
  QWidget::setVisible(v);
}

void ImageRPhiProjection::configure(char*& p, unsigned input, unsigned& output,
				ChannelDefinition* channels[], int* signatures, unsigned nchannels)
{
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels);
}

void ImageRPhiProjection::setup_payload(Cds& cds)
{
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
   (*it)->setup_payload(cds);
}

void ImageRPhiProjection::update()
{
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->update();
}

void ImageRPhiProjection::set_channel(int c) 
{ 
  _channel=c; 
}

void ImageRPhiProjection::plot()
{
  double f0 = _annulus->phi0();
  double f1 = _annulus->phi1();
  if (!(f0 < f1)) f1 += 2*M_PI;

  Ami::RPhiProjection* proj;

  if (_axis->checkedId()==0) { // R
    int r0 = int(_annulus->r_inner());
    int r1 = int(_annulus->r_outer());
    if (_norm->checkedId()==0) {
      Ami::DescTH1F desc(qPrintable(_title->text()),
			 "radius", "sum",
			 r1-r0+1, double(r0), double(r1+1));
      proj = new Ami::RPhiProjection(desc,
				     Ami::RPhiProjection::R, f0, f1,
				     _annulus->xcenter(), _annulus->ycenter());
    }
    else {
      Ami::DescProf desc(qPrintable(_title->text()),
			 "radius", "mean",
			 r1-r0+1, double(r0), double(r1+1), "");
      proj = new Ami::RPhiProjection(desc,
				     Ami::RPhiProjection::R, f0, f1,
				     _annulus->xcenter(), _annulus->ycenter());
    }
  }
  else {
    if (_norm->checkedId()==0) {
      Ami::DescTH1F desc(qPrintable(_title->text()),
			 "azimuth", "sum",
			 int(0.5*(_annulus->r_outer()+_annulus->r_inner())*(f1-f0)),
			 f0, f1);
      proj = new Ami::RPhiProjection(desc,
				     Ami::RPhiProjection::Phi,
				     _annulus->r_inner(), _annulus->r_outer(),
				     _annulus->xcenter(), _annulus->ycenter());
    }
    else {
      Ami::DescProf desc(qPrintable(_title->text()),
			 "azimuth", "mean",
			 int(0.5*(_annulus->r_outer()+_annulus->r_inner())*(f1-f0)),
			 f0, f1, "");
      proj = new Ami::RPhiProjection(desc,
				     Ami::RPhiProjection::Phi,
				     _annulus->r_inner(), _annulus->r_outer(),
				     _annulus->xcenter(), _annulus->ycenter());
    }
  }

  ProjectionPlot* plot = new ProjectionPlot(this,_title->text(), _channel, proj);
  _pplots.push_back(plot);

  connect(plot, SIGNAL(description_changed()), this, SLOT(configure_plot()));
  connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));

  emit changed();
}

void ImageRPhiProjection::remove_plot(QObject* obj)
{
  { ProjectionPlot* plot = static_cast<ProjectionPlot*>(obj);
    _pplots.remove(plot); }

  disconnect(obj, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
}

void ImageRPhiProjection::configure_plot()
{
  emit changed();
}
