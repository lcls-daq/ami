#include "ImageContourProjection.hh"

#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/RectangleCursors.hh"
#include "ami/qt/ProjectionPlot.hh"
#include "ami/qt/Display.hh"
#include "ami/qt/ImageFrame.hh"
#include "ami/qt/Contour.hh"

#include "ami/data/DescTH1F.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/Entry.hh"
#include "ami/data/ContourProjection.hh"

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

ImageContourProjection::ImageContourProjection(QWidget*           parent,
					       ChannelDefinition* channels[],
					       unsigned           nchannels, 
					       ImageFrame&        frame) :
  QtPWidget (parent),
  _channels (channels),
  _nchannels(nchannels),
  _channel  (0),
  _frame    (frame),
  _title    (new QLineEdit("Projection"))
{
  _rectangle = new RectangleCursors(_frame);
  _contour   = new Contour("X","f(X)",frame,*_rectangle);

  setWindowTitle("Contour Projection");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QComboBox* channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    channelBox->addItem(channels[i]->name());

  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* closeB = new QPushButton("Close");

//   QRadioButton* xaxisB = new QRadioButton("X");
//   QRadioButton* yaxisB = new QRadioButton("Y");
//   _axis = new QButtonGroup;
//   _axis->addButton(xaxisB,0);
//   _axis->addButton(yaxisB,1);
//   xaxisB->setChecked(true);
  //  _contour->setup("X","Y");
//   connect(xaxisB, SIGNAL(toggled(bool)), this, SLOT(use_xaxis(bool)));

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
  { QGroupBox* locations_box = new QGroupBox("Define Boundaries");
    locations_box->setToolTip("Define projection boundaries.");
    QVBoxLayout* layout2 = new QVBoxLayout;
    layout2->addWidget(_rectangle);
    locations_box->setLayout(layout2);
    layout->addWidget(locations_box); }
  { QGroupBox* plot_box = new QGroupBox("Plot");
    QVBoxLayout* layout1 = new QVBoxLayout;
    { QHBoxLayout* layout2 = new QHBoxLayout;
      layout2->addWidget(new QLabel("Title"));
      layout2->addWidget(_title);
      layout1->addLayout(layout2); }
    { QHBoxLayout* layout2 = new QHBoxLayout;
      layout2->addWidget(new QLabel("Project"));
      { QVBoxLayout* layout3 = new QVBoxLayout;
	layout3->addWidget(sumB);
	layout3->addWidget(meanB);
	layout2->addLayout(layout3); }
      layout2->addWidget(new QLabel("onto Y\' Axis : Y' = Y - f(X)"));
//       layout2->addWidget(new QLabel("onto"));
//       { QVBoxLayout* layout3 = new QVBoxLayout;
// 	layout3->addWidget(xaxisB);
// 	layout3->addWidget(yaxisB);
// 	layout2->addLayout(layout3); }
//       layout2->addWidget(new QLabel("axis"));
      layout2->addStretch();
      layout1->addLayout(layout2); }
    layout1->addWidget(_contour);
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
  
ImageContourProjection::~ImageContourProjection()
{
}

void ImageContourProjection::save(char*& p) const
{
  QtPWidget::save(p);

  QtPersistent::insert(p,_channel);
  QtPersistent::insert(p,_title->text());
//   QtPersistent::insert(p,_axis ->checkedId());
  QtPersistent::insert(p,_norm ->checkedId());
  _rectangle->save(p);
  _contour->save(p);

  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++) {
    QtPersistent::insert(p,QString("ProjectionPlot"));
    (*it)->save(p);
  }

  QtPersistent::insert(p,QString("EndImageContourProjection"));
}

void ImageContourProjection::load(const char*& p) 
{
  QtPWidget::load(p);
  
  _channel = QtPersistent::extract_i(p);
  _title->setText(QtPersistent::extract_s(p));
//   _axis ->button(QtPersistent::extract_i(p))->setChecked(true);
  _norm ->button(QtPersistent::extract_i(p))->setChecked(true);
  _rectangle->load(p);
  _contour->load(p);

  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    disconnect(*it, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _pplots.clear();

  QString name = QtPersistent::extract_s(p);
  while(name == QString("ProjectionPlot")) {
    ProjectionPlot* plot = new ProjectionPlot(this, p);
    _pplots.push_back(plot);
    connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));

    name = QtPersistent::extract_s(p);
  }

  if (name != QString("EndImageContourProjection"))
    printf("Error loading ImageContourProjection\n");
}

void ImageContourProjection::save_plots(const QString& p) const
{
  int i=1;
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->save_plots(QString("%1_%2").arg(p).arg(i++));
}

void ImageContourProjection::setVisible(bool v)
{
  if (v) {
    _frame.add_marker(*_rectangle);
    _frame.add_marker(*_contour);
  }
  else {
    _frame.remove_marker(*_rectangle);
    _frame.remove_marker(*_contour);
  }
  QWidget::setVisible(v);
}

void ImageContourProjection::configure(char*& p, unsigned input, unsigned& output,
				       ChannelDefinition* channels[], int* signatures, unsigned nchannels)
{
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels);
}

void ImageContourProjection::setup_payload(Cds& cds)
{
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
   (*it)->setup_payload(cds);
}

void ImageContourProjection::update()
{
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->update();
}

void ImageContourProjection::set_channel(int c) 
{ 
  _channel=c; 
}

void ImageContourProjection::plot()
{
  Ami::ContourProjection* proj;

//   if (_axis->checkedId()==0) { // X
//     unsigned xlo = unsigned(_rectangle->xlo());
//     unsigned xhi = unsigned(_rectangle->xhi());
//     unsigned ylo = unsigned(_rectangle->ylo());
//     unsigned yhi = unsigned(_rectangle->yhi());
//     if (_norm->checkedId()==0) {
//       Ami::DescTH1F desc(qPrintable(_title->text()),
// 			 "pixel", "sum",
// 			 xhi-xlo+1, xlo, xhi);
//       proj = new Ami::ContourProjection(desc, 
// 					Ami::ContourProjection::X, 
// 					xlo, xhi, ylo, yhi);
//     }
//     else {
//       Ami::DescProf desc(qPrintable(_title->text()),
// 			 "pixel", "mean",
// 			 xhi-xlo+1, xlo, xhi, "");
//       proj = new Ami::ContourProjection(desc, 
// 					Ami::ContourProjection::X, 
// 					xlo, xhi, ylo, yhi);
//     }
//   }
//   else { // Y


    Ami::Contour f = _contour->value();
    double ymin, ymax;
    f.extremes(_rectangle->xlo(), _rectangle->xhi(),
	       ymin, ymax);

    double y0 = (_rectangle->ylo() - ymin);
    double y1 = (_rectangle->yhi() - ymax+1);

    double xlo = (_rectangle->xlo());
    double xhi = (_rectangle->xhi());
    double ylo = (_rectangle->ylo());
    double yhi = (_rectangle->yhi());

    const AxisInfo& yinfo = *_frame.yinfo();
    int nby = yinfo.tick(y1-y0)+1;

    if (_norm->checkedId()==0) {
      Ami::DescTH1F desc(qPrintable(_title->text()),
			 "pixel", "sum",
			 nby, y0, y1);
      proj = new Ami::ContourProjection(desc, f,
					Ami::ContourProjection::Y, 
					xlo, xhi, ylo, yhi);
    }
    else {
      Ami::DescProf desc(qPrintable(_title->text()),
			 "pixel", "mean",
			 nby, y0, y1, "");
      proj = new Ami::ContourProjection(desc, f,
					Ami::ContourProjection::Y, 
					xlo, xhi, ylo, yhi);
    }
//   }
  
  ProjectionPlot* plot = new ProjectionPlot(this,_title->text(), _channel, proj);

  _pplots.push_back(plot);

  connect(plot, SIGNAL(description_changed()), this, SLOT(configure_plot()));
  connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));

  emit changed();
}

void ImageContourProjection::remove_plot(QObject* obj)
{
  { ProjectionPlot* plot = static_cast<ProjectionPlot*>(obj);
    _pplots.remove(plot); }

  disconnect(obj, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
}

void ImageContourProjection::configure_plot()
{
  emit changed();
}

// void ImageContourProject::use_xaxis(bool v)
// {
//   if (v) _contour->setup("Y","X");
//   else   _contour->setup("X","Y");
// }
