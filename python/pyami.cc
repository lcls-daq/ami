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

#include "ami/data/DescScan.hh"
#include "ami/data/EntryScan.hh"

#include "ami/data/EnvPlot.hh"
#include "ami/data/Single.hh"
#include "ami/data/Average.hh"

#include "ami/service/Ins.hh"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include <string>
#include <vector>

static const unsigned CDS_SUBNET_LO = 37;
static const unsigned CDS_SUBNET_HI = 44;
static const unsigned CDS_SUBNET_L2 = 10;

static const unsigned FEZ_SUBNET_LO = 20;
static const unsigned FEZ_SUBNET_HI = 27;
static const unsigned FEZ_SUBNET_L2 =  8;

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

class Info_pyami : public Pds::DetInfo {
public:
  Info_pyami(unsigned phy) { _log=Pds::Level::Source<<24; _phy=phy; }
  ~Info_pyami() {}
};

static int _parseargs(PyObject* args, PyObject* kwds, Ami::Python::ClientArgs& cl_args)
{
  if (!PyTuple_Check(args)) {
    char buff[64];
    sprintf(buff,"Ami Entry args type (%s) is not tuple",args->ob_type->tp_name);
    PyErr_SetString(PyExc_RuntimeError,buff);
    return -1;
  }

  unsigned index=0, index_n=0;
  PyObject* t;
  int sts;

  static const unsigned channel_default=0;
  unsigned phy=0, channel=channel_default;
  Info_pyami info(0x100);
  Ami::AbsOperator* op = 0;
  Ami::AbsFilter*   filter = 0;
  
  while(1) {
    { char* kwlist[] = {"name","entry_type",NULL};
      const char *name = 0, *entry_type=0;
      t = PyTuple_GetSlice(args,index,index_n=2);
      sts = PyArg_ParseTupleAndKeywords(t,kwds,"s|s",kwlist,
					&name, &entry_type);

      Py_DECREF(t);
      //
      //  Handle scalar variables (like diodes and BLD)
      //
      if (sts) {
	index = index_n;
	if (entry_type==0 || strcmp(entry_type,"Scalar")==0) {
	  op = new Ami::EnvPlot(Ami::DescScalar(name,"mean"));
	  break;
	}
        else if (strcmp(entry_type,"Scan")==0) {
	  unsigned nbins = 0;
          char* xtitle = 0;
	  char* ekwlist[] = {"xtitle","nbins",NULL};
	  t = PyTuple_GetSlice(args,index,index_n+=2);
	  sts = PyArg_ParseTupleAndKeywords(t,kwds,"sI",ekwlist,
					    &xtitle, &nbins);
          if (sts) {
            // printf( "Creating Scan %s, %s, %d\n" , name,xtitle,nbins);
            op = new Ami::EnvPlot(Ami::DescScan(name,xtitle,"mean",nbins));
            break;
          }
          return -1;
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
      t = PyTuple_GetSlice(args,index,index_n=2);
      sts = PyArg_ParseTupleAndKeywords(t,kwds,"II",kwlist,
					&phy, &channel);
      Py_DECREF(t);
      if (sts) {
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
    // printf("parsed filter from %s\n",filter_str);
  }

  cl_args.info    = info;
  cl_args.channel = channel;
  cl_args.filter  = filter;
  cl_args.op      = op;
  return 0;
}

static PyObject* _getentrylist(Ami::Python::Client* client)
{
  int sts = client->request_payload();
  if (sts==0) {
    std::vector<const Ami::Entry*> entries = client->payload();
    PyObject* o_list = PyList_New(entries.size());
    for(unsigned ie=0; ie<entries.size(); ie++) {
      const Ami::Entry* entry = entries[ie];
      if (entry == 0) {
	Py_DECREF(o_list);
	PyErr_SetString(PyExc_RuntimeError,"Failed to retrieve data.  Is variable in DAQ readout?");
	return NULL;
      }
      switch(entry->desc().type()) {
      case Ami::DescEntry::Scalar:
	{ const Ami::EntryScalar* s = static_cast<const Ami::EntryScalar*>(entry);
	  PyObject* o = PyDict_New();
	  PyDict_SetItemString(o,"type"   ,PyString_FromString("Scalar"));
	  PyDict_SetItemString(o,"entries",PyLong_FromDouble (s->entries()));
	  PyDict_SetItemString(o,"mean"   ,PyFloat_FromDouble(s->mean()));
	  PyDict_SetItemString(o,"rms"    ,PyFloat_FromDouble(s->rms()));
	  PyList_SetItem(o_list,ie,o); 
	  break; 
	}
      case Ami::DescEntry::TH1F:
	{ const Ami::EntryTH1F* s = static_cast<const Ami::EntryTH1F*>(entry);
	  PyObject* t = PyList_New(s->desc().nbins());
	  for(unsigned i=0; i<s->desc().nbins();i++)
	    PyList_SetItem(t,i,PyFloat_FromDouble(s->content(i)));

	  PyObject* o = PyDict_New();
	  PyDict_SetItemString(o,"type",   PyString_FromString("TH1F"));
	  PyDict_SetItemString(o,"uflow",  PyLong_FromDouble(s->info(Ami::EntryTH1F::Underflow)));
	  PyDict_SetItemString(o,"oflow",  PyLong_FromDouble(s->info(Ami::EntryTH1F::Overflow)));
	  PyDict_SetItemString(o,"data",   t);

	  Py_DECREF(t);
	  PyList_SetItem(o_list,ie,o);
	  break;
	}
      case Ami::DescEntry::Waveform:
	{ const Ami::EntryWaveform* s = static_cast<const Ami::EntryWaveform*>(entry);
	  PyObject* t = PyList_New(s->desc().nbins());
	  for(unsigned i=0; i<s->desc().nbins();i++)
	    PyList_SetItem(t,i,PyFloat_FromDouble(s->content(i)));

	  PyObject* o = PyDict_New();
	  PyDict_SetItemString(o,"type",   PyString_FromString("Waveform"));
	  PyDict_SetItemString(o,"entries",PyLong_FromDouble(s->info(Ami::EntryWaveform::Normalization)));
	  PyDict_SetItemString(o,"xlow",   PyFloat_FromDouble(s->desc().xlow()));
	  PyDict_SetItemString(o,"xhigh",  PyFloat_FromDouble(s->desc().xup ()));
	  PyDict_SetItemString(o,"data",   t);

	  Py_DECREF(t);
	  PyList_SetItem(o_list,ie,o);
	  break;
	}
      case Ami::DescEntry::Image:
	{ const Ami::EntryImage* s = static_cast<const Ami::EntryImage*>(entry);
	  PyObject* t;
	  PyObject* o;

	  if (s->desc().nframes()<=1) {
	    t = PyList_New(s->desc().nbinsy());  // rows
	    for(unsigned i=0; i<s->desc().nbinsy();i++) {
	      PyObject* col = PyList_New(s->desc().nbinsx());
	      for(unsigned j=0; j<s->desc().nbinsx();j++)
		PyList_SetItem(col,j,PyFloat_FromDouble(s->content(j,i)));
	      PyList_SetItem(t,i,col);
	    }
	    o = PyDict_New();
	    PyDict_SetItemString(o,"type",   PyString_FromString("Image"));
	    PyDict_SetItemString(o,"entries",PyLong_FromDouble (s->info(Ami::EntryImage::Normalization)));
	    PyDict_SetItemString(o,"offset", PyFloat_FromDouble(s->info(Ami::EntryImage::Pedestal)));
	    PyDict_SetItemString(o,"ppxbin", PyLong_FromLong(s->desc().ppxbin()));
	    PyDict_SetItemString(o,"ppybin", PyLong_FromLong(s->desc().ppybin()));
	    PyDict_SetItemString(o,"data",   t);
	  }
	  else {
	    t = PyList_New(s->desc().nframes());
	    for(unsigned k=0; k<s->desc().nframes(); k++) {
	      const Ami::SubFrame& f = s->desc().frame(k);
	      PyObject* rows = PyList_New(f.ny);
	      for(unsigned i=0; i<f.ny;i++) {
		PyObject* col = PyList_New(f.nx);
		for(unsigned j=0; j<f.nx;j++)
		  PyList_SetItem(col,j,PyFloat_FromDouble(s->content(f.x+j,f.y+i)));
		PyList_SetItem(rows,i,col);
	      }
	      PyList_SetItem(t,k,Py_BuildValue("ddO",f.x,f.y,rows));
	    }
	    o = PyDict_New();
	    PyDict_SetItemString(o,"type",   PyString_FromString("ImageArray"));
	    PyDict_SetItemString(o,"entries",PyLong_FromDouble (s->info(Ami::EntryImage::Normalization)));
	    PyDict_SetItemString(o,"offset", PyFloat_FromDouble(s->info(Ami::EntryImage::Pedestal)));
	    PyDict_SetItemString(o,"ppxbin", PyLong_FromLong(s->desc().ppxbin()));
	    PyDict_SetItemString(o,"ppybin", PyLong_FromLong(s->desc().ppybin()));
	    PyDict_SetItemString(o,"data",   t);

	  }
	  Py_DECREF(t);
	  PyList_SetItem(o_list,ie,o);
	  break;
	}
      case Ami::DescEntry::Scan:
	{ const Ami::EntryScan* s = static_cast<const Ami::EntryScan*>(entry);

	  PyObject* o = PyDict_New();
	  PyDict_SetItemString(o,"type",   PyString_FromString("Scan"));
	  PyDict_SetItemString(o,"nbins",  PyLong_FromDouble(s->desc().nbins()));
	  PyDict_SetItemString(o,"current",PyLong_FromDouble(s->info(Ami::EntryScan::Current)));
	  { PyObject* t = PyList_New(s->desc().nbins());
	    for(unsigned i=0; i<s->desc().nbins();i++)
	      PyList_SetItem(t,i,PyFloat_FromDouble(s->xbin(i)));
	    PyDict_SetItemString(o,"xbins",   t);
	    Py_DECREF(t);
	  }
	  { PyObject* t = PyList_New(s->desc().nbins());
	    for(unsigned i=0; i<s->desc().nbins();i++)
	      PyList_SetItem(t,i,PyFloat_FromDouble(s->nentries(i)));
	    PyDict_SetItemString(o,"yentries",   t);
	    Py_DECREF(t);
	  }
	  { PyObject* t = PyList_New(s->desc().nbins());
	    for(unsigned i=0; i<s->desc().nbins();i++)
	      PyList_SetItem(t,i,PyFloat_FromDouble(s->ysum(i)));
	    PyDict_SetItemString(o,"ysum",   t);
	    Py_DECREF(t);
	  }
	  { PyObject* t = PyList_New(s->desc().nbins());
	    for(unsigned i=0; i<s->desc().nbins();i++)
	      PyList_SetItem(t,i,PyFloat_FromDouble(s->y2sum(i)));
	    PyDict_SetItemString(o,"y2sum",   t);
	    Py_DECREF(t);
	  }
	  PyList_SetItem(o_list,ie,o);
	  break;
	}
      default:
	break;
      }
    }
    return o_list;
  }
  PyErr_SetString(PyExc_RuntimeError,"get timedout");
  return NULL;
}


//
//  amientry Object methods
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

static int amientry_init(amientry* self, PyObject* args, PyObject* kwds)
{
  std::vector<Ami::Python::ClientArgs> cl_argslist(1);
  if (_parseargs(args, kwds, cl_argslist[0]) < 0)
    return -1;

  PyErr_Clear();

  Ami::Python::Client* cl = new Ami::Python::Client(cl_argslist);

  int result = cl->initialize(*_discovery->allocate(*cl));

  if (result == Ami::Python::Client::Success) {
    self->client = cl;
    return 0;
  }
  else if (result == Ami::Python::Client::TimedOut) {
    PyErr_SetString(PyExc_RuntimeError,"Ami configure timeout");
  }
  else if (result == Ami::Python::Client::NoEntry) {
    PyErr_SetString(PyExc_RuntimeError,"Detector entry not found");
  }
  return -1;
}

static PyObject* amientry_get(PyObject* self, PyObject* args)
{
  amientry* e = reinterpret_cast<amientry*>(self);
  PyObject* o = _getentrylist(e->client);
  if (o && PyList_Check(o)) {
    PyObject* p = PyList_GetItem(o,0);
    Py_INCREF(p);
    Py_DECREF(o);
    return p;
  }
  return NULL;
}

static PyObject* amientry_clear(PyObject* self, PyObject* args)
{
  amientry* e = reinterpret_cast<amientry*>(self);
  e->client->reset();
  Py_INCREF(Py_None);
  return Py_None;
}

//
//  Register amientry methods
//
static PyMethodDef amientry_methods[] = {
  {"get"   , amientry_get   , METH_VARARGS, "Return the accumulated data"},
  {"clear" , amientry_clear , METH_VARARGS, "Clear the accumulated data"},
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
//  amientrylist Object methods
//
static void amientrylist_dealloc(amientrylist* self) 
{
  if (self->client) {
    delete self->client;
    self->client = 0;
  }

  self->ob_type->tp_free((PyObject*)self);
}

static PyObject* amientrylist_new(PyTypeObject* type, PyObject* args, PyObject* kwds) 
{
  amientrylist* self;

  self = (amientrylist*)type->tp_alloc(type,0);
  if (self != NULL) {
    self->phy     = 0;
    self->channel = 0;
    self->client  = 0;
  }

  return (PyObject*)self;
}

static int amientrylist_init(amientrylist* self, PyObject* argstuple, PyObject* kwds)
{
  PyObject* argslist = PyTuple_GetItem(argstuple,0);

  if (!PyList_Check(argslist)) {
    PyErr_SetString(PyExc_RuntimeError,"pyami.EntryList init arguments is not a list");
    return -1;
  }

  std::vector<Ami::Python::ClientArgs> cl_argslist(PyList_Size(argslist));

  for(unsigned i=0; i<PyList_Size(argslist); i++) {

    PyObject* args = PyList_GetItem(argslist,i);

    if (_parseargs(args, kwds, cl_argslist[i]) < 0)
      return -1;
  }

  PyErr_Clear();

  Ami::Python::Client* cl = new Ami::Python::Client(cl_argslist);

  int result = cl->initialize(*_discovery->allocate(*cl));

  if (result == Ami::Python::Client::Success) {
    self->client = cl;
    return 0;
  }
  else if (result == Ami::Python::Client::TimedOut) {
    PyErr_SetString(PyExc_RuntimeError,"Ami configure timeout");
  }
  else if (result == Ami::Python::Client::NoEntry) {
    PyErr_SetString(PyExc_RuntimeError,"Detector entry not found");
  }
  return -1;
}

static PyObject* amientrylist_get(PyObject* self, PyObject* args)
{
  amientrylist* e = reinterpret_cast<amientrylist*>(self);
  return _getentrylist(e->client);
}

static PyObject* amientrylist_clear(PyObject* self, PyObject* args)
{
  amientrylist* e = reinterpret_cast<amientrylist*>(self);
  e->client->reset();
  Py_INCREF(Py_None);
  return Py_None;
}

//
//  Register amientrylist methods
//
static PyMethodDef amientrylist_methods[] = {
  {"get"   , amientrylist_get   , METH_VARARGS, "Return the accumulated data"},
  {"clear" , amientrylist_clear , METH_VARARGS, "Clear the accumulated data"},
  {NULL},
};

//
//  Register amientrylist members
//
static PyMemberDef amientrylist_members[] = {
  {"phy"    , T_INT, offsetof(amientrylist, phy    ), 0, "detector identifier"},
  {"channel", T_INT, offsetof(amientrylist, channel), 0, "detector channel"},
  {NULL} 
};

static PyTypeObject amientrylist_type = {
  PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "pyami.EntryList",         /* tp_name */
    sizeof(amientrylist),      /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)amientrylist_dealloc, /*tp_dealloc*/
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
    "pyami EntryList objects",     /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    amientrylist_methods,      /* tp_methods */
    amientrylist_members,      /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)amientrylist_init,   /* tp_init */
    0,                         /* tp_alloc */
    amientrylist_new,          /* tp_new */
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
  unsigned ppinterface(0), mcinterface(0), servergroup;

  if (!PyArg_ParseTuple(args, "I|II", &servergroup,
                        &ppinterface, &mcinterface))
    return NULL;

  if (ppinterface==0 || mcinterface==0) {
    //
    //  Lookup these parameters from the available network interfaces
    //
    int fd;  
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      PyErr_SetString(PyExc_RuntimeError,"failed to lookup host interface");
      return NULL;
    }

    const int MaxRoutes = 5;
    struct ifreq ifrarray[MaxRoutes];
    memset(ifrarray, 0, sizeof(ifreq)*MaxRoutes);

    struct ifconf ifc;
    ifc.ifc_len = MaxRoutes * sizeof(struct ifreq);
    ifc.ifc_buf = (char*)ifrarray;
  
    if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) {
      PyErr_SetString(PyExc_RuntimeError,"failed to lookup host interfaces");
      close(fd);
      return NULL;
    }

    for (int i=0; i<MaxRoutes; i++) {
      struct ifreq* ifr = ifrarray+i;
      if (!ifr || !(((sockaddr_in&)ifr->ifr_addr).sin_addr.s_addr)) break;

      struct ifreq ifreq_flags = *ifr;
      if (ioctl(fd, SIOCGIFFLAGS, &ifreq_flags) < 0) {
        PyErr_SetString(PyExc_RuntimeError,"failed to lookup host interfaces");
        close(fd);
        return NULL;
      }

      int flags = ifreq_flags.ifr_flags;
      if ((flags & IFF_UP) && (flags & IFF_BROADCAST)) {
        struct ifreq ifreq_hwaddr = *ifr;
        if (ioctl(fd, SIOCGIFHWADDR, &ifreq_hwaddr) < 0) 
          continue;

        unsigned addr = htonl(((sockaddr_in&)ifr->ifr_addr).sin_addr.s_addr);
        unsigned subn = (addr>>8)&0xff;
        //      printf("Found addr %08x  subn %d\n",addr,subn);
        if ((subn>=CDS_SUBNET_LO &&
	     subn<=CDS_SUBNET_HI) ||
	    subn==CDS_SUBNET_L2) 
          ppinterface = addr;
        else if ((subn>=FEZ_SUBNET_LO && 
		  subn<=FEZ_SUBNET_HI) ||
		 subn==FEZ_SUBNET_L2)
          mcinterface = addr;
      }
    }
  }

  if (ppinterface==0) {
    PyErr_SetString(PyExc_RuntimeError,"failed to lookup host interface");
    return NULL;
  }
  if (mcinterface==0) {
    if (Ami::Ins::is_multicast(servergroup)) {
      PyErr_SetString(PyExc_RuntimeError,"failed to lookup group interface");
      return NULL;
    }
    else
      mcinterface = ppinterface;
  }

  if (_discovery)
    delete _discovery;

  _discovery = new Ami::Python::Discovery(ppinterface,
					  mcinterface,
					  servergroup);
  Py_INCREF(Py_None);
  return Py_None;
}

static unsigned colormap_jet[] = { 0x80, 0x84, 0x89, 0x8d, 0x92, 0x96, 0x9b, 0x9f, 0xa4, 0xa9, 0xad, 0xb2, 0xb6, 0xbb, 0xbf, 0xc4,
                                   0xc9, 0xcd, 0xd2, 0xd6, 0xdb, 0xdf, 0xe4, 0xe8, 0xed, 0xf2, 0xf6, 0xfb, 0xff, 0xff, 0xff, 0xff,
                                   0xff, 0x4ff, 0x8ff, 0xcff, 0x10ff, 0x14ff, 0x18ff, 0x1cff, 0x20ff, 0x24ff, 0x28ff, 0x2cff, 0x30ff, 0x34ff, 0x38ff, 0x3cff,
                                   0x40ff, 0x44ff, 0x48ff, 0x4cff, 0x50ff, 0x54ff, 0x58ff, 0x5cff, 0x60ff, 0x64ff, 0x68ff, 0x6cff, 0x70ff, 0x74ff, 0x78ff, 0x7cff,
                                   0x81ff, 0x85ff, 0x89ff, 0x8dff, 0x91ff, 0x95ff, 0x99ff, 0x9dff, 0xa1ff, 0xa5ff, 0xa9ff, 0xadff, 0xb1ff, 0xb5ff, 0xb9ff, 0xbdff,
                                   0xc1ff, 0xc5ff, 0xc9ff, 0xcdff, 0xd1ff, 0xd5ff, 0xd9ff, 0xddff, 0xe1fb, 0xe5f8, 0x2e9f5, 0x5edf2, 0x8f1ee, 0xcf5eb, 0xff9e8, 0x12fde5,
                                   0x15ffe1, 0x19ffde, 0x1cffdb, 0x1fffd8, 0x22ffd4, 0x26ffd1, 0x29ffce, 0x2cffcb, 0x2fffc7, 0x33ffc4, 0x36ffc1, 0x39ffbe, 0x3cffbb, 0x3fffb7, 0x43ffb4, 0x46ffb1,
                                   0x49ffae, 0x4cffaa, 0x50ffa7, 0x53ffa4, 0x56ffa1, 0x59ff9d, 0x5dff9a, 0x60ff97, 0x63ff94, 0x66ff90, 0x6aff8d, 0x6dff8a, 0x70ff87, 0x73ff83, 0x77ff80, 0x7aff7d,
                                   0x7dff7a, 0x80ff77, 0x83ff73, 0x87ff70, 0x8aff6d, 0x8dff6a, 0x90ff66, 0x94ff63, 0x97ff60, 0x9aff5d, 0x9dff59, 0xa1ff56, 0xa4ff53, 0xa7ff50, 0xaaff4c, 0xaeff49,
                                   0xb1ff46, 0xb4ff43, 0xb7ff3f, 0xbbff3c, 0xbeff39, 0xc1ff36, 0xc4ff33, 0xc7ff2f, 0xcbff2c, 0xceff29, 0xd1ff26, 0xd4ff22, 0xd8ff1f, 0xdbff1c, 0xdeff19, 0xe1ff15,
                                   0xe5ff12, 0xe8ff0f, 0xebff0c, 0xeeff08, 0xf2fd05, 0xf5f902, 0xf8f500, 0xfbf100, 0xffee00, 0xffea00, 0xffe600, 0xffe200, 0xffdf00, 0xffdb00, 0xffd700, 0xffd400,
                                   0xffd000, 0xffcc00, 0xffc800, 0xffc500, 0xffc100, 0xffbd00, 0xffba00, 0xffb600, 0xffb200, 0xffae00, 0xffab00, 0xffa700, 0xffa300, 0xffa000, 0xff9c00, 0xff9800,
                                   0xff9400, 0xff9100, 0xff8d00, 0xff8900, 0xff8600, 0xff8200, 0xff7e00, 0xff7a00, 0xff7700, 0xff7300, 0xff6f00, 0xff6c00, 0xff6800, 0xff6400, 0xff6000, 0xff5d00,
                                   0xff5900, 0xff5500, 0xff5100, 0xff4e00, 0xff4a00, 0xff4600, 0xff4300, 0xff3f00, 0xff3b00, 0xff3700, 0xff3400, 0xff3000, 0xff2c00, 0xff2900, 0xff2500, 0xff2100,
                                   0xff1d00, 0xff1a00, 0xff1600, 0xff1200, 0xfb0f00, 0xf60b00, 0xf20700, 0xed0300, 0xe80000, 0xe40000, 0xdf0000, 0xdb0000, 0xd60000, 0xd20000, 0xcd0000, 0xc90000,
                                   0xc40000, 0xbf0000, 0xbb0000, 0xb60000, 0xb20000, 0xad0000, 0xa90000, 0xa40000, 0x9f0000, 0x9b0000, 0x960000, 0x920000, 0x8d0000, 0x890000, 0x840000, 0x800000 };

static bool Parse_Int(PyObject* o, int& v)
{
  if (PyInt_Check(o))
    v = PyInt_AsLong(o);
  else if (PyFloat_Check(o))
    v = int(PyFloat_AsDouble(o));
  else
    return false;
  return true;
}

static PyObject*
pyami_map_image(PyObject *self, PyObject *args)
{
  PyObject* obuff  = 0;
  PyObject* shape  = 0;
  int       stride = 0;
  PyObject* slice = 0;
  PyObject* range = 0;
  const char* map = 0;

  if (!PyArg_ParseTuple(args,"OOIOs|O",&obuff,&shape,&stride,&slice,&map,&range)) 
    return NULL;

  PyBufferProcs* p = obuff->ob_type->tp_as_buffer;
  if (p==NULL) {
    PyErr_SetString(PyExc_RuntimeError,"Argument 0 is not a buffer object");
    return NULL;
  }

  const unsigned* mapv = 0;
  if (strcasecmp(map,"jet")==0)
    mapv = colormap_jet;
//   else if (strcasecmp(map,"thermal")==0)
//     mapv = colormap_thermal;

  if (mapv==0) {
    char buff[64];
    sprintf(buff,"failed to lookup color map %s",map);
    PyErr_SetString(PyExc_RuntimeError,buff);
    return NULL;
  }

  if (!PyTuple_Check(shape)) {
    PyErr_SetString(PyExc_RuntimeError,"Shape is not a tuple");
    return NULL;
  }
  
  int nrows = PyInt_AsLong(PyTuple_GetItem(shape,0));
  int ncols = PyInt_AsLong(PyTuple_GetItem(shape,1));

  int row0 = PyInt_AsLong(PyList_GetItem(PyList_GetItem(slice,0),0));
  int row1 = PyInt_AsLong(PyList_GetItem(PyList_GetItem(slice,0),1))+1;
  int col0 = PyInt_AsLong(PyList_GetItem(PyList_GetItem(slice,1),0));
  int col1 = PyInt_AsLong(PyList_GetItem(PyList_GetItem(slice,1),1))+1;

  if (row0 < 0) row0=0;
  if (row1 > nrows) row1=nrows;
  if (col0 < 0) col0=0;
  if (col1 > ncols) col1=ncols;

  int zmin=(unsigned(-1)>>1),zmax=(unsigned(-1)>>1)+1;
  if (range) {
    if (!PyList_Check(range)) {
      PyErr_SetString(PyExc_RuntimeError,"Range is not a list");
      return NULL;
    }
    if (PyList_Size(range)!=2) {
      PyErr_SetString(PyExc_RuntimeError,"Length of Range is not 2");
      return NULL;
    }

    if (!Parse_Int(PyList_GetItem(range,0),zmin) ||
        !Parse_Int(PyList_GetItem(range,1),zmax)) {
      PyErr_SetString(PyExc_RuntimeError,"Type of Range is not Int or Float");
      return NULL;
    }
  }      
  else {  // autorange
    int imin,jmin,imax,jmax;
    void* ptr;
    (*p->bf_getreadbuffer)(obuff,0,&ptr);
    int8_t* cptr = reinterpret_cast<int8_t*>(ptr);
    for(int j=row0; j<row1; j++) {
      int32_t* rowptr = reinterpret_cast<int32_t*>(cptr);
      for(int i=col0; i<col1; i++) {
        int v = rowptr[i];
        if (v < zmin) { zmin = v; imin=i; jmin=j; }
        if (v > zmax) { zmax = v; imax=i; jmax=j; }
      }
      cptr += stride;
    }
  }

  if (zmin == zmax) {
    zmin -= 1;
    zmax += 1;
  }
  
  double scale = 255./double(zmax-zmin);

  PyObject* out = PyList_New(row1-row0);
  void* ptr;
  (*p->bf_getreadbuffer)(obuff,0,&ptr);
  int8_t* cptr = reinterpret_cast<int8_t*>(ptr);
  for(int j=row0,jo=0; j<row1; j++,jo++) {
    PyObject* orow = PyList_New(col1-col0);
    int32_t* rowptr = reinterpret_cast<int32_t*>(cptr);
    for(int i=col0,io=0; i<col1; i++,io++) {
      double v = double(rowptr[i]-zmin)*scale;
      int iv = v < 0 ? 0 : v > 255 ? 255 : int(v);
      PyList_SetItem(orow, io, PyInt_FromLong(mapv[iv]));
    }
    PyList_SetItem(out,jo,orow);
    cptr += stride;
  }
  
  PyObject* orange = PyList_New(2);
  PyList_SetItem(orange,0,PyInt_FromLong(zmin));
  PyList_SetItem(orange,1,PyInt_FromLong(zmax));

  PyObject* result = PyTuple_New(2);
  PyTuple_SetItem(result,0,out);
  PyTuple_SetItem(result,1,orange);
  return result;
}

static PyMethodDef PyamiMethods[] = {
    {"connect"  , pyami_connect  , METH_VARARGS, "Connect to servers."},
    {"map_image", pyami_map_image, METH_VARARGS, "map image data to RGB color scale."},
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

  if (PyType_Ready(&amientrylist_type) < 0) {
    return; 
  }

  PyObject *m = Py_InitModule("pyami", PyamiMethods);
  if (m == NULL)
    return;

  _discovery = 0;

  Py_INCREF(&amientry_type);
  PyModule_AddObject(m, "Entry", (PyObject*)&amientry_type);

  Py_INCREF(&amientrylist_type);
  PyModule_AddObject(m, "EntryList", (PyObject*)&amientrylist_type);

  AmiError = PyErr_NewException("pyami.error", NULL, NULL);
  Py_INCREF(AmiError);
  PyModule_AddObject(m, "error", AmiError);
}

