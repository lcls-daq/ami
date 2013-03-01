#include "ami/qt/XYHistogramPlotDesc.hh"

#include "ami/qt/QtPersistent.hh"
#include "ami/qt/DescTH1F.hh"

#include "ami/data/DescTH1F.hh"
#include "ami/data/DescScalarRange.hh"
#include "ami/data/XYHistogram.hh"

#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Filter.hh"
#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/EntryScalarRange.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>

using namespace Ami::Qt;

enum { PlotSum, PlotMean };

XYHistogramPlotDesc::XYHistogramPlotDesc(QWidget* parent) :
  QWidget(parent),
  _output   (-1),
  _entry    (0)
{
  QVBoxLayout* layout1 = new QVBoxLayout;
  layout1->addStretch();
  layout1->addWidget(_desc = new DescTH1F("Pixel Values"));
  layout1->addStretch();
  setLayout(layout1);
}

XYHistogramPlotDesc::~XYHistogramPlotDesc()
{
}

void XYHistogramPlotDesc::save(char*& p) const
{
  XML_insert( p, "DescTH1F", "_desc", _desc->save(p) );
}

void XYHistogramPlotDesc::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.name == "_desc")
      _desc->load(p);
  XML_iterate_close(XYHistogramPlotDesc,tag);
}

Ami::XYHistogram* XYHistogramPlotDesc::desc(const char* title) const
{
  Ami::DescEntry* desc = 0;

  switch(_desc->method()) {
  case DescTH1F::Fixed:
    desc = new Ami::DescTH1F(title,
                             "pixel value", "pixels",
                             _desc->bins(), _desc->lo(), _desc->hi());
    break;
  case DescTH1F::Auto1:
    desc = new Ami::DescScalarRange(title,"events",
                                    DescScalarRange::MeanSigma,
                                    _desc->sigma(),
                                    _desc->nsamples(),
                                    _desc->bins());
    break;
  case DescTH1F::Auto2:
    desc = new Ami::DescScalarRange(title,"events",
                                    DescScalarRange::MinMax,
                                    _desc->extent(),
                                    _desc->nsamples(),
                                    _desc->bins());
    break;
  default:
    break;
  }
  Ami::XYHistogram* r = new Ami::XYHistogram(*desc);
  delete desc;
  return r;
}

void XYHistogramPlotDesc::setup_payload(Cds& cds)
{
}

void XYHistogramPlotDesc::configure(char*& p, unsigned input, unsigned& output,
			   ChannelDefinition* input_channels[], int* input_signatures, unsigned input_nchannels)
{
}

void XYHistogramPlotDesc::update()
{
}

