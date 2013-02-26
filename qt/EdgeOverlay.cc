#include "EdgeOverlay.hh"

#include "ami/qt/AxisInfo.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/PlotFactory.hh"
#include "ami/qt/QtBase.hh"
#include "ami/qt/QtPlot.hh"

#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/AbsTransform.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EdgeFinder.hh"

#include <QtGui/QLabel>
#include "qwt_plot.h"

using namespace Ami::Qt;

EdgeOverlay::EdgeOverlay(OverlayParent&   parent,
                         QtPlot&          plot,
                         unsigned         channel,
                         Ami::EdgeFinder* finder) :
  QtOverlay(parent),
  _frame   (&plot),
  _frame_name(plot._name),
  _channel (channel),
  _fcnt    (0)
{
  for (int i = 0; i < MAX_FINDERS; i++) {
    _finder[i] = 0;
    _plot[i] = 0;
    _order[i] = -1;
  }
  addfinder(finder);
}

EdgeOverlay::EdgeOverlay(OverlayParent& parent,
                         const char*&   p) :
  QtOverlay(parent),
  _frame   (0),
  _fcnt    (0)
{
  for (int i = 0; i < MAX_FINDERS; i++) {
    _finder[i] = 0;
    _plot[i] = 0;
    _order[i] = -1;
  }
  XML_iterate_open(p,tag)
    if (tag.name == "frame_name")
      _frame_name = QtPersistent::extract_s(p);
    else if (tag.name == "_channel")
      _channel = QtPersistent::extract_i(p);
    else if (tag.name == "_finder") {
      const char* b = (const char*)QtPersistent::extract_op(p);
      b += 2*sizeof(uint32_t);
      _finder[0] = new Ami::EdgeFinder(0.5,
                                       Ami::EdgeFinder::EdgeAlgorithm(Ami::EdgeFinder::halfbase2peak,
                                                                      true), 0.0, b);
      _fcnt = 1;
    } else if (tag.name.compare(0,7,"_finder") == 0) { /* _finderNNN */
        int i = atoi(tag.name.substr(7).c_str());
        Ami::EdgeFinder *f = loadfinder(p);
        if (i < MAX_FINDERS) {
            _finder[i] = f;
            if (i + 1 > _fcnt)
                _fcnt = i + 1;
        } else {
            printf("EdgeOverlay: unknown tag %s/%s\n", tag.element.c_str(), tag.name.c_str());
            delete f;
        }
    }
  XML_iterate_close(EdgeOverlay,tag);
}

EdgeOverlay::~EdgeOverlay()
{
  for (int i = 0; i < _fcnt; i++) {
    if (_finder[i]  ) delete _finder[i];
    if (_plot[i]    ) delete _plot[i];
  }
}

void EdgeOverlay::savefinder(Ami::EdgeFinder *f, char*& p) const
{
    XML_insert( p, "double",   "_threshold", QtPersistent::insert(p,f->threshold()) );
    XML_insert( p, "double",   "_baseline",  QtPersistent::insert(p,f->baseline()) );
    XML_insert( p, "int",      "_algorithm", QtPersistent::insert(p,f->algorithm()) );
    XML_insert( p, "double",   "_fraction",  QtPersistent::insert(p,f->fraction()) );
    XML_insert( p, "double",   "_deadtime",  QtPersistent::insert(p,f->deadtime()) );
    XML_insert( p, "DescEntry","_output",    QtPersistent::insert(p,f->desc(), f->desc_size()));
    XML_insert( p, "Parameter","_parameter", QtPersistent::insert(p,f->parameter()));
}

Ami::EdgeFinder *EdgeOverlay::loadfinder(const char*& p) 
{
  double thresh = 0.0, base = 0.0;
  int alg = Ami::EdgeFinder::EdgeAlgorithm(Ami::EdgeFinder::halfbase2peak, true);
  double deadtime = 0.0;
  double fraction = 0.5;
  const Ami::DescEntry *desc = NULL;
  EdgeFinder::Parameter parm = EdgeFinder::Location;

  XML_iterate_open(p,tag)
    if (tag.name == "_threshold")
      thresh = Ami::Qt::QtPersistent::extract_d(p);
    else if (tag.name == "_baseline")
      base = Ami::Qt::QtPersistent::extract_d(p);
    else if (tag.name == "_algorithm")
      alg = Ami::Qt::QtPersistent::extract_i(p);
    else if (tag.name == "_deadtime")
      deadtime = Ami::Qt::QtPersistent::extract_d(p);
    else if (tag.name == "_fraction")
      fraction = Ami::Qt::QtPersistent::extract_d(p);
    else if (tag.name == "_output") {
      desc = (const Ami::DescEntry*)QtPersistent::extract_op(p);
    }
    else if (tag.name == "_parameter")
      parm = Ami::EdgeFinder::Parameter(Ami::Qt::QtPersistent::extract_i(p));
  XML_iterate_close(EdgeFinder,tag);

  if (desc)
    return new Ami::EdgeFinder(fraction, thresh, base, alg, deadtime, *desc, parm);
  else
      return NULL;
}

void EdgeOverlay::save(char*& p) const
{
  char* buff = new char[8*1024];

  XML_insert( p, "QString", "_frame_name", QtPersistent::insert(p,_frame_name));

  XML_insert( p, "int", "_channel",
              QtPersistent::insert(p,(int)_channel) );

  /*
   * We used to save a single binary EdgeFinder with the following command:
   *   XML_insert( p, "EdgeFinder", "_finder",
   *              QtPersistent::insert(p, buff, (char*)_finder[0]->serialize(buff)-buff) );
   * load() will still support this.
   */
  for (int i = 0; i < _fcnt; i++) {
      sprintf(buff, "_finder%d", i);
      XML_insert(p, "EdgeFinder", buff, savefinder(_finder[i], p));
  }

  delete[] buff;
}

void EdgeOverlay::load(const char*& p) 
{
}

void EdgeOverlay::dump(FILE* f)          const { _plot[0]->dump(f); }
void EdgeOverlay::dump(FILE* f, int idx) const { _plot[idx]->dump(f); }

#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

void EdgeOverlay::setup_payload(Cds& cds)
{
  for (int i = 0; i < _fcnt; i++) {
    Ami::Entry* entry = cds.entry(_output_signature + i);
    if (entry) {
      if (_plot[i] && !_req[i].changed()) {
        _plot[i]->entry(*entry);
      }
      else { 
        if (_plot[i])
          delete _plot[i];

        _plot[i] = PlotFactory::plot(entry->desc().name(),*entry,
                                     Ami::AbsTransform::null(),
                                     Ami::AbsTransform::null(),
                                     QColor(0,0,0));

        if (_frame)
          _attach(i);
      }
      if (!_frame && (_frame = QtPlot::lookup(_frame_name)))
        _attach(i);
   }
    else {
      if (_output_signature + i >=0)
        printf("%s output_signature %d not found\n",_finder[i]->output().name(), _output_signature + i);
      if (_plot[i]) {
          delete _plot[i];
          _plot[i] = 0;
      }
    }
  }
}

void EdgeOverlay::addfinder(Ami::EdgeFinder *f)
{
  _finder[_fcnt++] = f;
}

void EdgeOverlay::configure(char*& p, unsigned input, unsigned& output,
			   ChannelDefinition* channels[], int* signatures, unsigned nchannels,
			   const AxisInfo& xinfo)
{
  unsigned input_signature = signatures[_channel];

  ConfigureRequest *r;
  for (int i = 0; i < _fcnt; i++) {
    r = new (p) ConfigureRequest(ConfigureRequest::Create,
                                 ConfigureRequest::Analysis,
                                 input_signature,
                                 -1,
                                 *channels[_channel]->filter().filter(),
                                 *_finder[i]);
    p += r->size();
    _req[i].request(*r, output);
    if (i==0)
      _output_signature = r->output();
  }
}

void EdgeOverlay::update()
{
  for (int i = 0; i < _fcnt; i++) {
    if (_plot[i]) {
      _plot[i]->update();
    }
  }
}

void EdgeOverlay::_attach(int i)
{
  if (_order[i]<0) {
    _order[i] = _frame->_frame->itemList().size();
    if (i==0)
      attach(*_frame);
  }

  _plot[i]->set_color(_order[i]-1);
  _plot[i]->attach(_frame->_frame);
  _frame->set_style();
}

const QtBase* EdgeOverlay::base() const 
{
  for(int i=0; i<_fcnt; i++) 
    if (_plot[i])
      return _plot[i];
  return 0;
}
