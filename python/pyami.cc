#include <Python.h>
#include <structmember.h>

#include "ami/python/Client.hh"
#include "ami/python/Discovery.hh"
#include "ami/python/pyami.hh"

#include "ami/data/RawFilter.hh"
#include "ami/data/FeatureRange.hh"
#include "ami/data/LogicAnd.hh"
#include "ami/data/LogicOr.hh"

#include "ami/data/DescScalar.hh"
#include "ami/data/EntryScalar.hh"

#include "ami/data/DescTH1F.hh"
#include "ami/data/EntryTH1F.hh"

#include "ami/data/DescWaveform.hh"
#include "ami/data/EntryWaveform.hh"

#include "ami/data/DescImage.hh"
#include "ami/data/EntryImage.hh"

#include "ami/data/EnvPlot.hh"
#include "ami/data/Single.hh"
#include "ami/data/Average.hh"

#include <string>

static PyObject* AmiError;

static Ami::Python::Discovery* _discovery;

static Ami::AbsFilter* parse_filter(std::string str)
{
  if (str[0]!='(') {
    int m1 = str.find_first_of('<');
    int m2 = str.find_last_of ('<');
    return new Ami::FeatureRange(str.substr(m1+1,m2-m1-1).c_str(),
				 strtod(str.substr(0,m1).c_str(),0),
				 strtod(str.substr(m2+1).c_str(),0));
  }
  else {
    //  break into binary operator and its operands
    int level=0;
    int pos=0;
    while(level) {
      pos=str.find_first_of("()",pos);
      if (str[pos++]=='(') 
	level++;
      else
	level--;
    }
    Ami::AbsFilter* a = parse_filter(str.substr(0,pos));
    Ami::AbsFilter* b = parse_filter(str.substr(pos+1));
    if (str[pos]=='&')
      return new Ami::LogicAnd(*a,*b);
    else
      return new Ami::LogicOr (*a,*b);
  }
}

static Ami::AbsFilter* parse_filter(const char* str)
{
  if (str==0)
    return new Ami::RawFilter;

  return parse_filter(std::string(str));
}

//
//  Object methods
//

static void amientry_dealloc(amientry* self)
{
  if (self->client) {
    delete self->client;
    self->client = 0;
  }

  self->ob_type->tp_free((PyObject*)self);
}

static PyObject* amientry_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  amientry* self;

  self = (amientry*)type->tp_alloc(type,0);
  if (self != NULL) {
    self->phy     = 0;
    self->channel = 0;
    self->client  = 0;
  }

  return (PyObject*)self;
}

class Info_pyami : public Pds::DetInfo {
public:
  Info_pyami(unsigned phy) { _log=Pds::Level::Source<<24; _phy=phy; }
  ~Info_pyami() {}
};

static int amientry_init(amientry* self, PyObject* args, PyObject* kwds)
{
  unsigned index=0, index_n=0;
  PyObject* t;
  int sts;

  static const unsigned channel_default=1024;
  unsigned phy=0, channel=channel_default;
  Info_pyami info(0x100);
  Ami::AbsOperator* op = 0;
  Ami::AbsFilter*   filter = 0;
  
  while(1) {
    { char* kwlist[] = {"name","entry_type",NULL};
      const char *name = 0, *entry_type=0;
      t = PyTuple_GetSlice(args,index,index_n+=2);
      sts = PyArg_ParseTupleAndKeywords(t,kwds,"s|s",kwlist,
					&name, &entry_type);
      Py_DECREF(t);
      //
      //  Handle scalar variables (like diodes and BLD)
      //
      if (sts) {
	index = index_n;
	if (entry_type==0 || strcmp(entry_type,"Scalar")==0) {
	  op = new Ami::EnvPlot(Ami::DescScalar(name,"mean",""));
	  break;
	}
	else if (strcmp(entry_type,"TH1F")==0) {
	  unsigned nbins = 0;
	  float    range_lo = 0;
	  float    range_hi = 0;
	  char* ekwlist[] = {"nbins","range_lo","range_hi",NULL};
	  t = PyTuple_GetSlice(args,index,index_n+=3);
	  sts = PyArg_ParseTupleAndKeywords(t,kwds,"Iff",ekwlist,
					    &nbins, &range_lo, &range_hi);
	  Py_DECREF(t);
	  if (sts) {
	    index = index_n;
	    op = new Ami::EnvPlot(Ami::DescTH1F(name,name,"events",nbins,range_lo,range_hi));
	    break;
	  }
	}
	return -1;
      }
    }
    { char* kwlist[] = {"det_id","channel",NULL};
      t = PyTuple_GetSlice(args,index,index_n+=2);
      sts = PyArg_ParseTupleAndKeywords(t,kwds,"I|I",kwlist,
					&phy, &channel);
      Py_DECREF(t);
      if (sts) {
	if (channel==channel_default)
	  index++;
	else
	  index = index_n;

	info = Info_pyami(phy);
	op   = new Ami::Average;
	break;
      }
    }
    return -1;
  }

  { char* kwlist[] = {"filter",NULL};
    const char* filter_str = 0;
    t = PyTuple_GetSlice(args,index,index_n+=1);
    sts = PyArg_ParseTupleAndKeywords(t,kwds,"|s",kwlist,&filter_str);
    Py_DECREF(t);
    filter = parse_filter(filter_str);
  }

  Ami::Python::Client* cl = new Ami::Python::Client(info, 
						    channel,
						    filter,
						    op);
  
  cl->managed(*_discovery->allocate(*cl));

  self->client = cl;

  return 0;
}

static PyObject* get(PyObject* self, PyObject* args)
{
  amientry* e = reinterpret_cast<amientry*>(self);

  int sts = e->client->request_payload();
  if (sts==0) {
    const Ami::Entry* entry = e->client->payload();
    switch(entry->desc().type()) {
    case Ami::DescEntry::Scalar:
      { const Ami::EntryScalar* s = static_cast<const Ami::EntryScalar*>(entry);
	return Py_BuildValue("sIdd",
			     "Scalar",
			     unsigned(s->entries()),
			     s->mean(),
			     s->rms()); }
    case Ami::DescEntry::TH1F:
      { const Ami::EntryTH1F* s = static_cast<const Ami::EntryTH1F*>(entry);
	PyObject* t = PyTuple_New(s->desc().nbins());
	for(unsigned i=0; i<s->desc().nbins();i++) {
	  PyObject* o = Py_BuildValue("d",s->content(i));
	  PyTuple_SetItem(t,i,o);
	}
	PyObject* result = Py_BuildValue("sddO",
					 "TH1F",
					 s->info(Ami::EntryTH1F::Underflow),
					 s->info(Ami::EntryTH1F::Overflow),
					 t);
	Py_DECREF(t);
	return result;
      }
    case Ami::DescEntry::Waveform:
      { const Ami::EntryWaveform* s = static_cast<const Ami::EntryWaveform*>(entry);
	PyObject* t = PyTuple_New(s->desc().nbins());
	for(unsigned i=0; i<s->desc().nbins();i++) {
	  PyObject* o = Py_BuildValue("d",s->content(i));
	  PyTuple_SetItem(t,i,o);
	}
	PyObject* result = Py_BuildValue("sddO",
					 "Waveform",
					 s->info(Ami::EntryWaveform::Normalization),
					 s->desc().xlow(),
					 s->desc().xup(),
					 t);
	Py_DECREF(t);
	return result;
      }
    case Ami::DescEntry::Image:
      { const Ami::EntryImage* s = static_cast<const Ami::EntryImage*>(entry);
	PyObject* t;
	PyObject* result;

	if (s->desc().nframes()<=1) {
	  t = PyTuple_New(s->desc().nbinsy());  // rows
	  for(unsigned i=0; i<s->desc().nbinsy();i++) {
	    PyObject* col = PyTuple_New(s->desc().nbinsx());
	    for(unsigned j=0; j<s->desc().nbinsx();j++)
	      PyTuple_SetItem(col,j,Py_BuildValue("d",s->content(j,i)));
	    PyTuple_SetItem(t,i,col);
	  }
	  result = Py_BuildValue("sddddO",
				 "Image",
				 s->info(Ami::EntryImage::Normalization),
				 s->info(Ami::EntryImage::Pedestal),
				 s->desc().ppxbin(),
				 s->desc().ppybin(),
				 t);
	}
	else {
	  t = PyTuple_New(s->desc().nframes());
	  for(unsigned k=0; k<s->desc().nframes(); k++) {
	    const Ami::SubFrame& f = s->desc().frame(k);
	    PyObject* rows = PyTuple_New(f.ny);
	    for(unsigned i=0; i<f.ny;i++) {
	      PyObject* col = PyTuple_New(f.nx);
	      for(unsigned j=0; j<f.nx;j++)
		PyTuple_SetItem(col,j,Py_BuildValue("d",s->content(f.x+j,f.y+i)));
	      PyTuple_SetItem(rows,i,col);
	    }
	    PyTuple_SetItem(t,k,Py_BuildValue("ddO",f.x,f.y,rows));
	  }
	  result = Py_BuildValue("sddddO",
				 "ImageArray",
				 s->info(Ami::EntryImage::Normalization),
				 s->info(Ami::EntryImage::Pedestal),
				 s->desc().ppxbin(),
				 s->desc().ppybin(),
				 t);
	}
	Py_DECREF(t);
	return result;
      }
    default:
      break;
    }
  }

  return Py_BuildValue("s","None");
//   Py_INCREF(Py_None);
//   return Py_None;
}

//
//  Register amientry methods
//
static PyMethodDef amientry_methods[] = {
  {"get"   , get   , METH_VARARGS, "Return the accumulated data"},
  {NULL},
};

//
//  Register amientry members
//
static PyMemberDef amientry_members[] = {
  {"phy"    , T_INT, offsetof(amientry, phy    ), 0, "detector identifier"},
  {"channel", T_INT, offsetof(amientry, channel), 0, "detector channel"},
  {NULL} 
};

static PyTypeObject amientry_type = {
  PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "pyami.Entry",             /* tp_name */
    sizeof(amientry),          /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)amientry_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "pyami Entry objects",     /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    amientry_methods,          /* tp_methods */
    amientry_members,          /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)amientry_init,   /* tp_init */
    0,                         /* tp_alloc */
    amientry_new,              /* tp_new */
};
    
//
//  Module methods
//
//    Setup multicast interface, point-to-point interface
//    Setup server group
//    Connect to servers
//

static PyObject*
pyami_connect(PyObject *self, PyObject *args)
{
  unsigned ppinterface, mcinterface, servergroup;

  if (!PyArg_ParseTuple(args, "III", &ppinterface, &mcinterface, &servergroup))
    return NULL;

  if (_discovery)
    delete _discovery;

  _discovery = new Ami::Python::Discovery(ppinterface,
					  mcinterface,
					  servergroup);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyMethodDef PyamiMethods[] = {
    {"connect",  pyami_connect, METH_VARARGS, "Connect to servers."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

//
//  Module initialization
//
PyMODINIT_FUNC
initpyami(void)
{
  if (PyType_Ready(&amientry_type) < 0) {
    return; 
  }

  PyObject *m = Py_InitModule("pyami", PyamiMethods);
  if (m == NULL)
    return;

  _discovery = 0;

  Py_INCREF(&amientry_type);
  PyModule_AddObject(m, "Entry", (PyObject*)&amientry_type);

  AmiError = PyErr_NewException("pyami.error", NULL, NULL);
  Py_INCREF(AmiError);
  PyModule_AddObject(m, "error", AmiError);
}

