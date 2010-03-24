#include "ami/qt/XYProjectionPlotDesc.hh"

#include "ami/qt/QtPersistent.hh"
#include "ami/qt/RectangleCursors.hh"

#include "ami/data/DescTH1F.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/XYProjection.hh"

#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>

using namespace Ami::Qt;

enum { PlotSum, PlotMean };

XYProjectionPlotDesc::XYProjectionPlotDesc(QWidget* parent,
					   const RectangleCursors& r) :
  QWidget(parent),
  _rectangle(r)
{
  QRadioButton* xaxisB = new QRadioButton("X");
  QRadioButton* yaxisB = new QRadioButton("Y");
  _axis = new QButtonGroup;
  _axis->addButton(xaxisB,0);
  _axis->addButton(yaxisB,1);
  xaxisB->setChecked(true);

  QRadioButton* sumB  = new QRadioButton("sum");
  QRadioButton* meanB = new QRadioButton("mean");
  _norm = new QButtonGroup;
  _norm->addButton(sumB ,PlotSum);
  _norm->addButton(meanB,PlotMean);
  sumB->setChecked(true);

  QVBoxLayout* layout1 = new QVBoxLayout;
  layout1->addStretch();
  { QHBoxLayout* layout2 = new QHBoxLayout;
    layout2->addWidget(new QLabel("Project"));
    { QVBoxLayout* layout3 = new QVBoxLayout;
      layout3->addWidget(sumB);
      layout3->addWidget(meanB);
      layout2->addLayout(layout3); }
    layout2->addWidget(new QLabel("onto"));
    { QVBoxLayout* layout3 = new QVBoxLayout;
      layout3->addWidget(xaxisB);
      layout3->addWidget(yaxisB);
      layout2->addLayout(layout3); }
    layout2->addWidget(new QLabel("axis"));
    layout2->addStretch(); 
    layout1->addLayout(layout2); }
  layout1->addStretch();
  setLayout(layout1);
}

XYProjectionPlotDesc::~XYProjectionPlotDesc()
{
}

void XYProjectionPlotDesc::save(char*& p) const
{
  QtPersistent::insert(p, _axis->checkedId());
  QtPersistent::insert(p, _norm->checkedId());
}

void XYProjectionPlotDesc::load(const char*& p)
{
  _axis->button(QtPersistent::extract_i(p))->setChecked(true);
  _norm->button(QtPersistent::extract_i(p))->setChecked(true);
}

Ami::XYProjection* XYProjectionPlotDesc::desc(const char* title) const
{
  Ami::XYProjection* proj;

  if (_axis->checkedId()==0) { // X
    unsigned xlo = unsigned(_rectangle.xlo());
    unsigned xhi = unsigned(_rectangle.xhi());
    unsigned ylo = unsigned(_rectangle.ylo());
    unsigned yhi = unsigned(_rectangle.yhi());
    if (_norm->checkedId()==0) {
      Ami::DescTH1F desc(title,
			 "pixel", "sum",
			 xhi-xlo+1, xlo, xhi);
      proj = new Ami::XYProjection(desc, Ami::XYProjection::X, ylo, yhi);
    }
    else {
      Ami::DescProf desc(title,
			 "pixel", "mean",
			 xhi-xlo+1, xlo, xhi, "");
      proj = new Ami::XYProjection(desc, Ami::XYProjection::X, ylo, yhi);
    }
  }
  else { // Y
    unsigned ylo = unsigned(_rectangle.ylo());
    unsigned yhi = unsigned(_rectangle.yhi());
    unsigned xlo = unsigned(_rectangle.xlo());
    unsigned xhi = unsigned(_rectangle.xhi());
    if (_norm->checkedId()==0) {
      Ami::DescTH1F desc(title,
			 "pixel", "sum",
			 yhi-ylo+1, ylo, yhi);
      proj = new Ami::XYProjection(desc, Ami::XYProjection::Y, xlo, xhi);
    }
    else {
      Ami::DescProf desc(title,
			 "pixel", "mean",
			 yhi-ylo+1, ylo, yhi, "");
      proj = new Ami::XYProjection(desc, Ami::XYProjection::Y, xlo, xhi);
    }
  }
  
  return proj;
}
