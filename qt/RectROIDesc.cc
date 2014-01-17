#include "ami/qt/RectROIDesc.hh"

#include "ami/qt/RectangleCursors.hh"
#include "ami/qt/Rect.hh"
#include "ami/qt/RectROI.hh"
#include "ami/qt/ImageFrame.hh"
#include "ami/qt/QtPWidget.hh"

#include <QtGui/QPushButton>
#include <QtGui/QComboBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>

using namespace Ami::Qt;

RectROIDesc::RectROIDesc(QtPWidget& p,
			 ImageFrame& frame,
			 unsigned nchannels) 
  : QWidget(0), _frame(frame), _nchannels(nchannels)
{
  _rectangle = new RectangleCursors(_frame, &p);
  
  _roiButton = new QPushButton("New");
  _roiBox = new QComboBox;
  new_roi();

  QVBoxLayout* layout = new QVBoxLayout;
  QGroupBox* locations_box = new QGroupBox("Region of Interest");
  locations_box->setToolTip("Define projection boundaries.");
  QVBoxLayout* layout2 = new QVBoxLayout;
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(new QLabel("Select"));
    layout1->addWidget(_roiBox);
    layout1->addWidget(_roiButton);
    layout1->addStretch();
    layout2->addLayout(layout1); }
  layout2->addWidget(_rectangle);
  locations_box->setLayout(layout2);
  layout->addWidget(locations_box); 
  setLayout(layout);

  connect(_rectangle, SIGNAL(changed()),      this, SLOT(update_range()));
  connect(_rectangle, SIGNAL(changed()),      this, SIGNAL(rchanged()));
  connect(_rectangle, SIGNAL(edited()),       this, SIGNAL(changed()));
  connect(_rectangle, SIGNAL(done()),         &p  , SLOT(front()));
  connect(_rectangle, SIGNAL(done()),         this, SIGNAL(changed()));
  connect(_roiButton, SIGNAL(clicked()),      this, SLOT(new_roi()));
  connect(_roiBox   , SIGNAL(currentIndexChanged(int)), this, SLOT(select_roi(int)));
  connect(_roiBox   , SIGNAL(currentIndexChanged(int)), this, SIGNAL(roi_changed(int)));
}

RectROIDesc::~RectROIDesc()
{
  for(unsigned i=0; i<_rect.size(); i++)
    delete _rect[i];
  _rect.clear();

  for(unsigned i=0; i<_rois.size(); i++)
    delete _rois[i];
  _rois.clear();
}

void RectROIDesc::save(char*& p) const
{
  XML_insert(p, "RectangleCursors", "_rectangle", _rectangle->save(p) );

  for(unsigned i=0; i<_rect.size(); i++) {
    XML_insert(p, "Rect", "_rect", _rect[i]->save(p) );
  }
  for(unsigned i=0; i<_rois.size(); i++) {
    XML_insert(p, "RectROI", "_rois", _rois[i]->save(p) );
  }
}

void RectROIDesc::load(const char*& p) 
{
  for(unsigned i=0; i<_rect.size(); i++)
    delete _rect[i];
  _rect.clear();

  for(unsigned i=0; i<_rois.size(); i++)
    delete _rois[i];
  _rois.clear();

  _roiBox->clear();

  XML_iterate_open(p,tag)
    if (tag.name == "_rectangle")
      _rectangle->load(p);
    else if (tag.name == "_rect") {
      Rect* r = new Rect;
      r->load(p); 
      _rect.push_back(r);
    }
    else if (tag.name == "_rois") {
      int rect = _rois.size()/_nchannels;
      QString name = QString("ROI%1").arg(rect);
      unsigned channel = _rois.size()%_nchannels;
      if (channel==0)
        _roiBox->addItem(name);
      RectROI* roi = new RectROI(this, name, channel, *_rect[rect]);
      roi->load(p);
      _rois.push_back(roi);
      connect(roi, SIGNAL(changed()), this, SIGNAL(changed()));
    }
  XML_iterate_close(RectROIDesc,tag);

  update_range();
}

void RectROIDesc::save_plots(const QString& p) const
{
  for(unsigned i=0; i<_rois.size(); i++) {
    QString q = QString("%1_ROI%2_%3").arg(p).arg(i/4).arg(i%4);
    _rois[i]->save_plots(q);
  }
}

void RectROIDesc::configure(char*& p, unsigned input, unsigned& output,
			    ChannelDefinition* channels[], int* signatures, unsigned nchannels)
{
  for(std::vector<RectROI*>::iterator it=_rois.begin(); it!=_rois.end(); it++) {
    (*it)->configure(p,input,output,channels,signatures,nchannels);
  }
}

void RectROIDesc::update()
{
  for(unsigned i=0; i<_rois.size(); i++)
    _rois[i]->update();
}

void RectROIDesc::setup_payload(Ami::Cds& cds)
{
  for(unsigned i=0; i<_rois.size(); i++) 
    _rois[i]->setup_payload(cds);
}

void RectROIDesc::update_range()
{
  _frame.replot();

  if (_roiBox->currentIndex()>=0)
    _rectangle->save(*_rect[_roiBox->currentIndex()]);
}
 
void RectROIDesc::new_roi()
{
  int i=_rois.size()/_nchannels;
  _rect.push_back(new Rect());
  QString name = QString("ROI%1").arg(i);
  _roiBox->addItem(name);
  for(unsigned j=0; j<_nchannels; j++) {
    RectROI* roi = new RectROI(this, name,j,*_rect[i]);
    _rois.push_back(roi);
    connect(roi, SIGNAL(changed()), this, SIGNAL(changed()));
  }
  _roiBox->setCurrentIndex(i);
}

void RectROIDesc::select_roi(int i)
{
  if (i>=0)
    _rectangle->load(*_rect[i]);
}

RectROI& RectROIDesc::roi(unsigned ichannel) { return *_rois[_roiBox->currentIndex()*_nchannels + ichannel]; }

void RectROIDesc::showEvent(QShowEvent* ev)
{
  QWidget::showEvent(ev);
  _frame.add_marker(*_rectangle);
  update_range();
}

void RectROIDesc::hideEvent(QHideEvent* ev)
{
  QWidget::hideEvent(ev);
  _frame.remove_marker(*_rectangle);
  update_range();
}
