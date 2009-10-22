#include "EnvPlot.hh"

#include "ami/qt/AxisArray.hh"
#include "ami/qt/PlotFactory.hh"
#include "ami/qt/QtTH1F.hh"
#include "ami/qt/QtChart.hh"
#include "ami/qt/QtProf.hh"
#include "ami/qt/QtScan.hh"
#include "ami/qt/Path.hh"

#include "ami/data/AbsTransform.hh"
#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EnvPlot.hh"
#include "ami/data/RawFilter.hh"

#include <QtGui/QLabel>
#include "qwt_plot.h"

namespace Ami {
  namespace Qt {
    class NullTransform : public Ami::AbsTransform {
    public:
      ~NullTransform() {}
      double operator()(double x) const { return x; }
    };
  };
};

using namespace Ami::Qt;

static NullTransform noTransform;

EnvPlot::EnvPlot(QWidget*         parent,
		 const QString&   name,
		 DescEntry*       desc,
		 int              index0,
		 const QString&   var) :
  QtPlot   (parent, name),
  _desc    (desc),
  _index0  (index0),
  _var     (var),
  _output_signature  (0),
  _plot    (0)
{
}

EnvPlot::EnvPlot(QWidget*     parent,
		 const char*& p) :
  QtPlot   (parent,p),
  _output_signature(0),
  _plot    (0)
{
  char* buff = new char[sizeof(DescProf)];
  DescEntry* desc = (DescEntry*)buff;
  memcpy(buff, p, sizeof(DescEntry));
  memcpy(buff+sizeof(DescEntry), p+sizeof(DescEntry), desc->size()-sizeof(DescEntry));
  p += desc->size();

#define CASEENTRY(type) case DescEntry::type: _desc = new Desc##type(*static_cast<Desc##type*>(desc)); break;

  switch(desc->type()) {
    CASEENTRY(TH1F)
    CASEENTRY(Prof)
    CASEENTRY(Scan)
    CASEENTRY(Scalar)
    default: break;
  }
  delete[] buff;

  _index0 = QtPersistent::extract_i(p);
  _var    = QtPersistent::extract_s(p);
}

EnvPlot::~EnvPlot()
{
  delete _desc;
  if (_plot    ) delete _plot;
}

void EnvPlot::save(char*& p) const
{
  QtPlot::save(p);

  memcpy(p, _desc, _desc->size()); p += _desc->size();
  QtPersistent::insert(p,_index0);
  QtPersistent::insert(p,_var);
}


void EnvPlot::load(const char*& p)
{
}

void EnvPlot::_dump(FILE* f) const { _plot->dump(f); }

#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

void EnvPlot::setup_payload(Cds& cds)
{
  if (_plot) delete _plot;
    
  Ami::Entry* entry = cds.entry(_output_signature);
  if (entry) {
    switch(entry->desc().type()) {
    case Ami::DescEntry::TH1F: 
      _plot = new QtTH1F(_name,*static_cast<const Ami::EntryTH1F*>(entry),
			 noTransform,noTransform,QColor(0,0,0));
      break;
    case Ami::DescEntry::Scalar:  // create a chart from a scalar
      _plot = new QtChart(_name,*static_cast<const Ami::EntryScalar*>(entry),
			  200,QColor(0,0,0));
      break;
    case Ami::DescEntry::Prof: 
      _plot = new QtProf(_name,*static_cast<const Ami::EntryProf*>(entry),
			 noTransform,noTransform,QColor(0,0,0));
      break;
    case Ami::DescEntry::Scan: 
      _plot = new QtScan(_name,*static_cast<const Ami::EntryScan*>(entry),
			 noTransform,noTransform,QColor(0,0,0));
      break;
    default:
      printf("EnvPlot type %d not implemented yet\n",entry->desc().type()); 
      return;
    }
    _plot->attach(_frame);
    printf("%s found signature %d created type %d\n",qPrintable(_name),_output_signature,entry->desc().type());
  }
  else
    printf("%s output_signature %d not found\n",qPrintable(_name),_output_signature);
}

void EnvPlot::configure(char*& p, unsigned input, unsigned& output)
{
  Ami::EnvPlot op(*_desc, _index0, qPrintable(_var));
  
  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						  ConfigureRequest::Discovery,
						  input,
						  _output_signature = ++output,
						  RawFilter(), op);
  p += r.size();
}

void EnvPlot::update()
{
  if (_plot) {
    _plot->update();
    _counts->setText(QString("Np %1").arg(_plot->normalization()));
    emit redraw();
  }
}
