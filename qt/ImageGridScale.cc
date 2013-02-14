#include "ami/qt/ImageGridScale.hh"

#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/ImageFrame.hh"
#include "ami/qt/CrossHair.hh"
//#include "ami/qt/CrossHairDelta.hh"
#include "ami/qt/Path.hh"

#include "ami/data/Cds.hh"
#include "ami/data/EntryImage.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QRadioButton>
#include <QtGui/QPushButton>
#include <QtGui/QButtonGroup>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

#include <errno.h>

using namespace Ami::Qt;

enum { Pixels, Millimeters };

ImageGridScale::ImageGridScale(ImageFrame& frame, bool grab) :
  QGroupBox("XY"),
  _frame    (frame),
  _scaled   (false),
  _scalex   (1),
  _scaley   (1)
{
  QRadioButton* pixelsB = new QRadioButton("pixels");
  QRadioButton* phyB    = new QRadioButton("mms");
  _group   = new QButtonGroup;
  pixelsB->setChecked(!_scaled);
  phyB   ->setChecked( _scaled);
  _group->addButton(pixelsB,Pixels);
  _group->addButton(phyB   ,Millimeters);

  QPushButton* saveB = new QPushButton("Save");
  QPushButton* loadB = new QPushButton("Load");

  setAlignment(::Qt::AlignHCenter);
  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* lh = new QHBoxLayout;
    lh->addWidget(pixelsB);
    lh->addStretch();
    lh->addWidget(phyB);
    layout->addLayout(lh); }
  { layout->addLayout(_clayout = new QGridLayout);
    _clayout->setVerticalSpacing(0); }
  { QHBoxLayout* l = new QHBoxLayout;
    l->addStretch();
    l->addWidget(saveB);
    l->addWidget(loadB);
    l->addStretch();
    layout->addLayout(l); }
  
  setLayout(layout);

  CrossHair::layoutHeader(*_clayout); _nrows=1;
  _cross_hairs.push_back( new CrossHair(*this, *_clayout, _nrows++, grab) );
  _cross_hairs.push_back( new CrossHair(*this, *_clayout, _nrows++, grab) );
//   _delta = new CrossHairDelta(*_clayout, _nrows++,
// 			      *_cross_hairs.front(),
// 			      *_cross_hairs.back());

  connect(phyB, SIGNAL(toggled(bool)), this, SLOT(phy_scale(bool)));
  connect(saveB, SIGNAL(clicked()), this, SLOT(save_list()));
  connect(loadB, SIGNAL(clicked()), this, SLOT(load_list()));
}
  
ImageGridScale::~ImageGridScale()
{
}

void ImageGridScale::save(char*& p) const
{
  for(std::list<CrossHair*>::const_iterator it=_cross_hairs.begin(); it!=_cross_hairs.end(); it++)
    XML_insert( p, "CrossHair", "_cross_hairs", (*it)->save(p) );
}

void ImageGridScale::load(const char*& p) 
{
  std::list<CrossHair*>::iterator iter=_cross_hairs.begin();
  XML_iterate_open(p,tag)
    if (tag.name == "_cross_hairs" && iter!=_cross_hairs.end())
      (*(iter++))->load(p);
  XML_iterate_close(ImageGridScale,tag);
}

void ImageGridScale::update()
{
}

void ImageGridScale::setup_payload(Cds& cds)
{
  for(int i=0, n=0; n<cds.totalentries(); i++) {
    const Entry* e = cds.entry(i);
    if (e) {
      if (e->desc().type() == Ami::DescEntry::Image) {
	_scalex = static_cast<const DescImage&>(e->desc()).mmppx();
	_scaley = static_cast<const DescImage&>(e->desc()).mmppy();
        if (_scalex==0) {
          _group->button(Pixels     )->setChecked(true);
          _group->button(Millimeters)->setEnabled(false);
          _scaled=false;
        }
        else {
          _group->button(1)->setEnabled(true);
        }
	phy_scale(_scaled);
	return;
      }
      n++;
    }
  }
  printf("ImageGridScale::setup_payload found no image\n");
}

void ImageGridScale::phy_scale(bool v) 
{ 
  _scaled = v;
  for(std::list<CrossHair*>::iterator it=_cross_hairs.begin();
      it!=_cross_hairs.end(); it++)
    if (v)
      (*it)->set_scale(_scalex,_scaley);
    else
      (*it)->set_scale(1.,1.);

  if (v)
    _frame.set_grid_scale(_scalex,_scaley);
  else
    _frame.set_grid_scale(1.,1.);
}

void ImageGridScale::setVisible(bool v)
{
  /*
  for(std::list<CrossHair*>::iterator it=_cross_hairs.begin();
      it!=_cross_hairs.end(); it++)
    (*it)->setVisible(v);
  */
  QWidget::setVisible(v);
}

ImageFrame& ImageGridScale::frame() { return _frame; }

static const unsigned MaxConfigSize = 0x1000;

void ImageGridScale::save_list()
{
  char* buffer = new char[MaxConfigSize];
  char* p = buffer;
  save(p);

  Path::saveAmiFile(this, buffer, p-buffer);

  delete[] buffer;
}

void ImageGridScale::load_list()
{
  // get the file 
  QString fname = QFileDialog::getOpenFileName(this,"Load Setup from File (.ami)",
                 Path::base(), "*.ami");
  FILE* f = fopen(qPrintable(fname),"r");
  if (!f) {
    QString msg = QString("Error opening %1 : %2").arg(fname).arg(strerror(errno));
    QMessageBox::critical(this,"Load Error",msg);
    return;
  }

  char* buffer = new char[MaxConfigSize];
  int size = fread(buffer,1,MaxConfigSize,f);
  fclose(f);

  size += sprintf(buffer+size,"</ImageGridScale>");

  const char* p = buffer;
  load(p);

  delete[] buffer;
}
