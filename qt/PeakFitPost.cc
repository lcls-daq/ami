#include "ami/qt/PeakFitPost.hh"

#include "ami/qt/AxisInfo.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/PFPostParent.hh"

#include "ami/data/PeakFitPlot.hh"
#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/DescEntry.hh"

using namespace Ami::Qt;

PeakFitPost::PeakFitPost(unsigned          channel,
			 Ami::PeakFitPlot* input,
                         PFPostParent*     parent) :
  _channel (channel),
  _input   (input),
  _parent  (parent)
{
}

PeakFitPost::PeakFitPost(const char*& p) :
  _parent  (0)
{
  load(p);
}

PeakFitPost::~PeakFitPost()
{
  if (_parent)
    _parent->remove_peakfit_post(this); 
  if (_input)
    delete _input;
}

void PeakFitPost::save(char*& p) const
{
  char* buff = new char[8*1024];
  XML_insert(p, "int"        , "_channel", QtPersistent::insert(p,(int)_channel) );
  XML_insert(p, "PeakFitPlot", "_input"  , QtPersistent::insert(p,buff,(char*)_input->serialize(buff)-buff) );
  delete[] buff;
}

void PeakFitPost::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.name == "_channel")
      _channel = QtPersistent::extract_i(p);
    else if (tag.name == "_input") {
      const char* b = (const char*)QtPersistent::extract_op(p);
      b += 2*sizeof(uint32_t);
      _input = new Ami::PeakFitPlot(b);
    }
  XML_iterate_close(PeakFitPost,tag);
}

void PeakFitPost::configure(char*& p, unsigned input, unsigned& output,
			   ChannelDefinition* channels[], int* signatures, unsigned nchannels,
			   const AxisInfo& xinfo, ConfigureRequest::Source source)
{
  unsigned channel = _channel;
  unsigned input_signature = signatures[channel];

  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						  source,
						  input_signature,
						  -1,
						  *channels[channel]->filter().filter(),
						  *_input);
  p += r.size();
  _req.request(r,output);
  _output_signature = r.output();
}

