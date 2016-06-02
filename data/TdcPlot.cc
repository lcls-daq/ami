#include "TdcPlot.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EntryRef.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryTH2F.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/data/Cds.hh"
#include "ami/data/FeatureExpression.hh"
#include "ami/data/valgnd.hh"

#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/psddl/acqiris.ddl.h"

typedef Pds::Acqiris::TdcDataV1 AcqTdcDataType;
typedef Pds::Acqiris::TdcDataV1Marker  MarkerType;
typedef Pds::Acqiris::TdcDataV1Channel ChannelType;

#include <QtCore/QString>
#include <QtCore/QRegExp>

#include <stdio.h>

using namespace Ami;

static const double TC890_Period = 50e-12;

class TdcPlot::TdcVar : public Ami::Variable {
public:
  TdcVar(unsigned i) : _name(QString("Chan%1").arg(i+1)) {}
  TdcVar(const TdcVar& c) : _name(c._name) {}
  ~TdcVar() {}
public:
  double evaluate() const { return _v; }
  Variable* clone() const;
  const QString& name() const { return _name; }
public:
  void set(double v);
  bool used() const { return !_clones.empty(); }
private:
  QString _name;
  double  _v;
  mutable std::list<TdcPlot::TdcVar*> _clones;
};

Variable* TdcPlot::TdcVar::clone() const
{
  TdcPlot::TdcVar* c = new TdcPlot::TdcVar(*this);
  _clones.push_back(c); 
  return c; 
}

void TdcPlot::TdcVar::set(double v)
{
  _v=v; 
  for(std::list<TdcPlot::TdcVar*>::iterator it=_clones.begin(); it!=_clones.end(); it++)
    (*it)->set(v);
}

TdcPlot::TdcPlot(const DescEntry& output, const char* expr) :
  AbsOperator(AbsOperator::TdcPlot),
  _xterm     (0),
  _output    (0),
  _v         (true)
{
  strncpy_val(_expression , expr, EXPRESSION_LEN);
  memcpy_val (_desc_buffer, &output, output.size(),DESC_LEN);
}

TdcPlot::TdcPlot(const char*& p, const DescEntry& input) :
  AbsOperator(AbsOperator::TdcPlot),
  _mask      (0),
  _v         (true)
{
  _extract(p, _expression , EXPRESSION_LEN);
  _extract(p, _desc_buffer, DESC_LEN);

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);

  _output = EntryFactory::entry(o);

  //  Parse the name to get the channel math
  for(unsigned i=0; i<6; i++)
    _chan[i] = new TdcVar(i);

  printf("TdcPlot parsing %s\n",_expression);

  std::list<Variable*> vars;
  for(unsigned i=0; i<6; i++)
    vars.push_back(_chan[i]);

  QString new_expr(_expression);

  Expression parser(vars);
  
  int pos=0;
  if ((pos = new_expr.indexOf('%'))!=-1) {
    _yterm = parser.evaluate(new_expr.mid(0,pos));
    _xterm = parser.evaluate(new_expr.mid(pos+1));
  }
  else {
    _xterm = parser.evaluate(new_expr);
    _yterm = 0;
  }
  if (!_xterm) {
    printf("TdcPlot failed to parse %s\n",qPrintable(new_expr));
    _v = false;
  }

  for(unsigned i=0; i<6; i++)
    if (_chan[i]->used())
      _mask |= 1<<i;
}

TdcPlot::TdcPlot(const char*& p) :
  AbsOperator(AbsOperator::TdcPlot),
  _mask      (0),
  _xterm     (0),
  _output    (0),
  _v         (true)
{
  _extract(p, _expression , EXPRESSION_LEN);
  _extract(p, _desc_buffer, DESC_LEN);
}

TdcPlot::~TdcPlot()
{
  if (_xterm   ) {
    for(unsigned i=0; i<6; i++)
      delete _chan[i];
    if (_yterm) 
      delete _yterm;
    delete _xterm;
  }
  if (_output ) {
    delete _output;
  }
}

void TdcPlot::use()
{
  if (_xterm) _xterm->use();
  if (_yterm) _yterm->use();
}

const DescEntry& TdcPlot::_routput   () const 
{ 
  return _output ? _output->desc() : *reinterpret_cast<const DescEntry*>(_desc_buffer); 
}

void*      TdcPlot::_serialize(void* p) const
{
  _insert(p, _expression , EXPRESSION_LEN);
  _insert(p, _desc_buffer, DESC_LEN);
  return p;
}

Entry&     TdcPlot::_operate(const Entry& e) const
{
  if (!e.valid())
    return *_output;

  if (!_xterm)
    return *_output;

  const EntryRef* input  = static_cast<const EntryRef*>(&e);

  if (_routput().type() != DescEntry::TH2F)
    _output->reset();

  //
  //  Depends upon the list of hits terminating in a Bank switch marker
  //
  unsigned mask(0);
  const AcqTdcDataType* p = reinterpret_cast<const AcqTdcDataType*>(input->data());
  //
  //  ndarray interface doesn't know the size of the data
  //
  ndarray<const Pds::Acqiris::TdcDataV1_Item,1> a = p->data(0);
  for(unsigned j=0; true; j++) {
    if (a[j].source() == Pds::Acqiris::TdcDataV1_Item::AuxIO &&
        static_cast<const MarkerType&>(a[j]).type() < MarkerType::AuxIOMarker) 
      break;

    if (a[j].source() > Pds::Acqiris::TdcDataV1_Item::Comm && 
        a[j].source() < Pds::Acqiris::TdcDataV1_Item::AuxIO) {
      const ChannelType& c = static_cast<const ChannelType&>(a[j]);
      unsigned ch = a[j].source()-1;
      //  If the hit is valid and is one we're interested in
      if (!c.overflow() && (_mask&(1<<ch))) {
        double t = c.time();
        _chan[ch]->set(t);
        //  If we've seen at least one of each hit we need
        if ((mask |= 1<<ch)==_mask) {
          switch(_routput().type()) {
          case DescEntry::TH1F:
            static_cast<EntryTH1F*>(_output)->addcontent(1.,_xterm->evaluate());
            break;
          case DescEntry::TH2F:
            if (_yterm)
              static_cast<EntryTH2F*>(_output)->addcontent(1.,_xterm->evaluate(),_yterm->evaluate());
            break;
          case DescEntry::Image:
            if (_yterm) {
              EntryImage* o = static_cast<EntryImage*>(_output);
              double x = _xterm->evaluate();
              if (x>=0 && x<o->desc().nbinsx()) {
                double y = _yterm->evaluate();
                if (y>=0 && y<o->desc().nbinsy())
                  o->addcontent(1,unsigned(x),o->desc().nbinsy()-unsigned(y)-1);
              }
            }
            break;
          default:
            break;
          }
        }
      }
    }
  }

  switch(_routput().type()) {
  case DescEntry::TH1F:
    static_cast<EntryTH1F* >(_output)->addinfo(1,EntryTH1F ::Normalization); break;
  case DescEntry::TH2F:
    static_cast<EntryTH2F* >(_output)->addinfo(1,EntryTH2F ::Normalization); break;
  case DescEntry::Image:
    static_cast<EntryImage*>(_output)->addinfo(1,EntryImage::Normalization); break;
  default:
    break;
  }

  _output->valid(e.time());
  return *_output;
}

void TdcPlot::_invalid() { _output->invalid(); }
