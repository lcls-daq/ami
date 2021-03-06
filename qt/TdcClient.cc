#include "TdcClient.hh"

#include "ami/qt/Control.hh"
#include "ami/qt/Status.hh"
#include "ami/qt/Calculator.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/TdcPlot.hh"
#include "ami/qt/ProjectionPlot.hh"
#include "ami/qt/TwoDPlot.hh"
#include "ami/qt/DescTH1F.hh"
#include "ami/qt/DescTH2T.hh"
#include "ami/qt/ControlLog.hh"

#include "ami/client/ClientManager.hh"

#include "ami/data/ConfigureRequest.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/DescTH1F.hh"
#include "ami/data/DescTH2F.hh"
#include "ami/data/DescImage.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/Expression.hh"
#include "ami/data/TdcPlot.hh"

#include "ami/service/Socket.hh"
#include "ami/service/Semaphore.hh"

#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QGroupBox>
#include <QtGui/QComboBox>

#include <sys/types.h>
#include <sys/socket.h>

using namespace Ami::Qt;

static const int BufferSize = 0x8000;

TdcClient::TdcClient(QWidget* parent, const Pds::DetInfo& info, unsigned channel, const QString& name) :
  Ami::Qt::AbsClient(parent,info,channel),
  _title           (name),
  _input_signature (0),
  _output_signature(0),
  _request         (BufferSize),
  _description     (BufferSize),
  _cds             ("Client"),
  _manager         (0),
  _niovload        (5),
  _iovload         (new iovec[_niovload]),
  _sem             (new Semaphore(Semaphore::EMPTY)),
  _list_sem        (Semaphore::FULL),
  _throttled       (false),
  _reset           (false)
{
  setWindowTitle(QString("%1[*]").arg(name));
  setAttribute(::Qt::WA_DeleteOnClose, false);

  _control = new Control(*this,1.);
  _status  = new Status;

  //  QPushButton* filterB = new QPushButton("Filter");
  _filter = new Filter     (NULL,_title);

  _source_edit    = new QLineEdit("Chan1");
  _source_compose = new QPushButton("Select");

  _vsource_edit    = new QLineEdit("Chan1");
  _vsource_compose = new QPushButton("Select");
  QHBoxLayout* vsl = new QHBoxLayout;
  vsl->addWidget(new QLabel("vs"));
  vsl->addWidget(_vsource_edit);
  vsl->addWidget(_vsource_compose);

  _plot_desc_1d = new DescTH1F("Hist1D", true, true, true);
  _plot_desc_1d->button()->setEnabled(true);

  _plot_desc_2d = new DescTH2T(vsl);
  _plot_desc_2d->td_button()->setEnabled(true);
  _plot_desc_2d->im_button()->setEnabled(true);

  _plot_grp = new QButtonGroup;
  _plot_grp->addButton(_plot_desc_1d->button(),(int)TH1F);
  _plot_grp->addButton(_plot_desc_2d->td_button(),(int)TH2F);
  _plot_grp->addButton(_plot_desc_2d->im_button(),(int)Image);
  _plot_desc_1d->button()->setChecked(true);

  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* closeB = new QPushButton("Close");

  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(_control);
    layout1->addStretch();
    layout1->addWidget(_status);
    layout->addLayout(layout1); }
  { QGroupBox* channel_box = new QGroupBox("Source Channel");
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(_source_edit);
    layout1->addWidget(_source_compose);
    //    layout1->addWidget(filterB);
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(_plot_desc_1d->button());
    layout1->addWidget(_plot_desc_1d); 
    layout->addLayout(layout1); }
  { layout->addWidget(_plot_desc_2d); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(plotB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);

  //  connect(filterB   , SIGNAL(clicked()),   _filter, SLOT(show()));
  connect(_source_edit   , SIGNAL(editingFinished()), this, SLOT(validate_source()));
  connect(_source_compose, SIGNAL(clicked()),         this, SLOT(select_source()));
  connect(_vsource_edit   , SIGNAL(editingFinished()), this, SLOT(validate_vsource()));
  connect(_vsource_compose, SIGNAL(clicked()),         this, SLOT(select_vsource()));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));
  connect(this, SIGNAL(description_changed(int)), this, SLOT(_read_description(int)));
  connect((AbsClient*)this, SIGNAL(changed()), this, SLOT(update_configuration()));
}

TdcClient::~TdcClient() 
{
  if (_manager) delete _manager;
  delete[] _iovload;
  delete _filter;
}

const QString& TdcClient::title() const { return _title; }

void TdcClient::save(char*& p) const
{
  XML_insert(p, "QtPWidget", "self", QtPWidget::save(p) );

  XML_insert(p, "QLineEdit", "_source_edit", QtPersistent::insert(p,_source_edit->text()) );
  //  _filter->save(p);

  XML_insert(p, "DescTH1F", "_plot_desc_1d", _plot_desc_1d->save(p) );
  XML_insert(p, "DescTH2T", "_plot_desc_2d", _plot_desc_2d->save(p) );

  for(std::list<TdcPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    XML_insert(p, "TdcPlot", "_plots", (*it)->save(p) );
  }
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++) {
    XML_insert(p, "ProjectionPlot", "_pplots", (*it)->save(p) );
  }
  for(std::list<TwoDPlot*>::const_iterator it=_tplots.begin(); it!=_tplots.end(); it++) {
    XML_insert(p, "TwoDPlot", "_tplots", (*it)->save(p) );
  }

  XML_insert(p, "Control", "_control", _control->save(p) );
}

void TdcClient::load(const char*& p)
{
  _list_sem.take();

  for(std::list<TdcPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    disconnect(*it, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _plots.clear();
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    disconnect(*it, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _pplots.clear();
  for(std::list<TwoDPlot*>::const_iterator it=_tplots.begin(); it!=_tplots.end(); it++)
    disconnect(*it, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _tplots.clear();

  
  XML_iterate_open(p,tag)
    if      (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.name == "_source_edit")
      _source_edit->setText(QtPersistent::extract_s(p));
  //  _filter  ->load(p);
    else if (tag.name == "_plot_desc_1d")
      _plot_desc_1d->load(p);
    else if (tag.name == "_plot_desc_2d")
      _plot_desc_2d->load(p);
    else if (tag.name == "_plots") {
      TdcPlot* plot = new TdcPlot(this, p);
      _plots.push_back(plot);
      connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
    }
    else if (tag.name == "_pplots") {
      ProjectionPlot* plot = new ProjectionPlot(this, p);
      _pplots.push_back(plot);
      connect(plot, SIGNAL(description_changed()), this, SIGNAL(changed()));
      connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
    }
    else if (tag.name == "_tplots") {
      TwoDPlot* plot = new TwoDPlot(this, p);
      _tplots.push_back(plot);
      connect(plot, SIGNAL(description_changed()), this, SIGNAL(changed()));
      //      connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
      connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
    }
    else if (tag.name == "_control")
      _control->load(p);
  XML_iterate_close(TdcClient,tag);

  _list_sem.give();

  update_configuration();
}

void TdcClient::snapshot(const QString& p) const
{
  int i=1;
  for(std::list<TdcPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->_snapshot(QString("%1_%2.png").arg(p).arg(i++));
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->_snapshot(QString("%1_%2.png").arg(p).arg((*it)->_name));
  for(std::list<TwoDPlot*>::const_iterator it=_tplots.begin(); it!=_tplots.end(); it++)
    (*it)->_snapshot(QString("%1_%2.png").arg(p).arg((*it)->_name));
}

void TdcClient::save_plots(const QString& p) const
{
  int i=1;
  for(std::list<TdcPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    QString s = QString("%1_%2.dat").arg(p).arg(i++);
    FILE* f = fopen(qPrintable(s),"w");
    if (f) {
      (*it)->dump(f);
      fclose(f);
    }
  }
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++) {
    QString s = QString("%1_%2.dat").arg(p).arg((*it)->_name);
    (*it)->save_plots(s);
  }
  for(std::list<TwoDPlot*>::const_iterator it=_tplots.begin(); it!=_tplots.end(); it++) {
    QString s = QString("%1_%2.dat").arg(p).arg((*it)->_name);
    (*it)->save_plots(s);
  }
}

void TdcClient::reset_plots()
{
  _reset=true;
  update_configuration(); 
}

void TdcClient::connected()
{
  if (_manager) {
    _status->set_state(Status::Connected);
    _manager->discover();
  }
}

void TdcClient::discovered(const DiscoveryRx& rx)
{
  _status->set_state(Status::Discovered);
  printf("Tdc Discovered\n");

  char channel_name [128];
  strcpy(channel_name ,qPrintable(_title));
  const DescEntry* e = rx.entry(channel_name);
  if (e) {
    _input_signature = e->signature();
    //    setWindowModified(!e->recorded());
  }
  else {
    printf("%s not found\n", channel_name);
    _input_signature = -1;
  }

  _manager->configure();
}

int  TdcClient::configure       (iovec* iov) 
{
  _status->set_state(Status::Configured);
  printf("Configure\n");

  char* p = _request.reset();

  if (_reset) {
    _reset=false;
    ConfigureRequest& req = *new (p) ConfigureRequest(ConfigureRequest::Reset,
						      ConfigureRequest::Analysis,
						      _input_signature);
    p = _request.extend(req.size());
  }
  else {
    _list_sem.take();

    for(std::list<TdcPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
      (*it)->configure(p,_input_signature,_output_signature);
      _request.extend(p);
    }
    for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++) {
      (*it)->configure(p,_input_signature,_output_signature, ConfigureRequest::Discovery);
      _request.extend(p);
    }
    for(std::list<TwoDPlot*>::const_iterator it=_tplots.begin(); it!=_tplots.end(); it++) {
      (*it)->configure(p,_input_signature,_output_signature, ConfigureRequest::Discovery);
      _request.extend(p);
    }
    _list_sem.give();
  }

  iov[0].iov_base = _request.base();
  iov[0].iov_len  = _request.extent();
  return 1;
}

int  TdcClient::configured      () 
{
  printf("Tdc Configured\n");
  return 0; 
}

int  TdcClient::read_description(Socket& socket,int len)
{
  _description.reset();
  _description.reserve(len);
  int size = socket.read(_description.base(),len);

  if (size<0) {
    printf("Read error in Ami::Qt::Client::read_description.\n");
    return 0;
  }

  _description.extend(size);

  emit description_changed(size);

  _sem->take();

  return size;
}

void TdcClient::_read_description(int size)
{
  _cds.reset();

  const char* payload = _description.base();
  const char* const end = payload + size;
  //  dump(payload, size);

  //  { const Desc* e = reinterpret_cast<const Desc*>(payload);
  //    printf("Cds %s\n",e->name()); }
  payload += sizeof(Desc);

  while( payload < end ) {
    const DescEntry* desc = reinterpret_cast<const DescEntry*>(payload);
    if (desc->size()==0) {
      printf("read_description size==0\n");
      break;
    }
    Entry* entry = EntryFactory::entry(*desc);
    _cds.add(entry, desc->signature());
    payload += desc->size();
//     printf("Received desc %s signature %d\n",desc->name(),desc->signature());
  }

  if (_cds.totalentries()>_niovload) {
    delete[] _iovload;
    _iovload = new iovec[_niovload=_cds.totalentries()];
  }
  _cds.payload(_iovload);

  for(std::list<TdcPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->setup_payload(_cds);

  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->setup_payload(_cds);

  for(std::list<TwoDPlot*>::const_iterator it=_tplots.begin(); it!=_tplots.end(); it++)
    (*it)->setup_payload(_cds);

  _status->set_state(Status::Described);

  _sem->give();
}

int  TdcClient::read_payload     (Socket& socket,int)
{
  int nbytes = 0;
  if (_status->state() == Status::Requested) {
    nbytes = socket.readv(_iovload,_cds.totalentries());
  }
  else {
    //
    //  Add together each server's contribution
    //
    printf("Ami::Qt::Client::read_payload: multiple server processing not implemented\n");
  }
  _status->set_state(Status::Received);
  return nbytes;
}

bool TdcClient::svc             () const { return false; }

void TdcClient::process         () 
{
  //
  //  Perform client-side processing
  //
  _list_sem.take();

  for(std::list<TdcPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->update();
  
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->update();
  
  for(std::list<TwoDPlot*>::const_iterator it=_tplots.begin(); it!=_tplots.end(); it++)
    (*it)->update();

  _list_sem.give();
  
  _status->set_state(Status::Processed);
}

void TdcClient::managed(ClientManager& mgr)
{
  _manager = &mgr;
  show();
  printf("TdcClient connecting\n");
  _manager->connect();
}

void TdcClient::request_payload()
{
  if (_plots.size()==0 && _pplots.size()==0 && _tplots.size()==0) return;

  if (_status->state() == Status::Described ||
      _status->state() == Status::Processed) {
    _throttled = false;
    _status->set_state(Status::Requested);
    _manager->request_payload();
  }
  else if (_status->state() == Status::Requested &&
           !_throttled) {
    _status->set_state(Status::Throttled);
    _throttled = true;
    QString msg("TdcClient request payload throttled");
    printf("%s\n",qPrintable(msg));
    _manager->dump_throttle();
    ControlLog::instance().appendText(msg);
  }
}

void TdcClient::update_configuration()
{
  if (_manager)
    _manager->configure();
}

void TdcClient::plot()
{
  //  Replace channel names with indices
  QString expr(_source_edit->text());

  switch(_plot_grp->checkedId()) {
  case TdcClient::TH1F:
    { Ami::DescTH1F desc(qPrintable(expr),
                         qPrintable(expr), "hits",
                         _plot_desc_1d->bins(),
                         _plot_desc_1d->lo(),
                         _plot_desc_1d->hi(),
                         _plot_desc_1d->normalize(),
                         _plot_desc_1d->aggregate());
      QString ptitle(expr);

      Ami::TdcPlot* op = new Ami::TdcPlot(desc,qPrintable(expr));
      
      ProjectionPlot* plot = new ProjectionPlot(this,
                                                ptitle,
                                                0,
                                                op);
      _list_sem.take();
      _pplots.push_back(plot);
      _list_sem.give();
      
      connect(plot, SIGNAL(description_changed()), this, SIGNAL(changed()));
      connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));

      break; }
  case TdcClient::TH2F:
    { expr += QString("%")+_vsource_edit->text();
      Ami::DescTH2F* desc = new Ami::DescTH2F(qPrintable(expr),
                                              qPrintable(_vsource_edit->text()),
                                              qPrintable( _source_edit->text()),
                                              _plot_desc_2d->xbins(),
                                              _plot_desc_2d->xlo(),
                                              _plot_desc_2d->xhi(),
                                              _plot_desc_2d->ybins(),
                                              _plot_desc_2d->ylo(),
                                              _plot_desc_2d->yhi(),
                                              false);
      TdcPlot* plot = new TdcPlot(this,
                                  expr,
                                  *_filter->filter(),
                                  desc,
                                  expr);
      _list_sem.take();
      _plots.push_back(plot);
      _list_sem.give();

      connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));

      break; }
  case TdcClient::Image:
    { QString iexpr = "((" + _source_edit->text() + ')' + Expression::subtract() + 
        Expression::constant(_plot_desc_2d->ylo()) + ')' +
        Expression::multiply() + Expression::constant(512./(_plot_desc_2d->yhi()-_plot_desc_2d->ylo()));
      iexpr += "%((" + _vsource_edit->text() + ')' + Expression::subtract() + 
        Expression::constant(_plot_desc_2d->xlo()) + ')' +
        Expression::multiply() + Expression::constant(512./(_plot_desc_2d->xhi()-_plot_desc_2d->xlo()));
      
      expr += QString("%") + _vsource_edit->text();
      Ami::DescImage desc(qPrintable(expr), 512, 512, 1, 1, 0, 0, false);
      Ami::TdcPlot* op = new Ami::TdcPlot(desc,qPrintable(iexpr));

      TwoDPlot* plot = new TwoDPlot(this,
                                    expr,
                                    op);

      _list_sem.take();
      _tplots.push_back(plot);
      _list_sem.give();

      connect(plot, SIGNAL(description_changed()), this, SIGNAL(changed()));
      //      connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
      connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));

      break; }
  default:
    break;
  }
      
  emit changed();
}

void TdcClient::remove_plot(QObject* obj)
{
  _list_sem.take();
  _plots .remove(static_cast<TdcPlot*>(obj));
  _pplots.remove(static_cast<ProjectionPlot*>(obj));
  _tplots.remove(static_cast<TwoDPlot*>(obj));
  _list_sem.give();

  disconnect(obj, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));

  delete obj;
}

void TdcClient::select_source () { _select_source(_source_edit); }
void TdcClient::select_vsource() { _select_source(_vsource_edit); }

void TdcClient::_select_source(QLineEdit* source_edit)
{
  static QChar _exponentiate(0x005E);
  static QChar _multiply    (0x00D7);
  static QChar _divide      (0x00F7);
  static QChar _add         (0x002B);
  static QChar _subtract    (0x002D);

  QStringList vars;
  for(unsigned i=0; i<6; i++)
    vars << QString("Chan%1").arg(i+1);

  QStringList varops; varops << _subtract;
  QStringList conops; conops << _multiply << _divide << _add << _subtract;

  Calculator* c = new Calculator(this,
                                 "Tdc Source","",
				 vars,
				 varops,
				 conops);
  if (c->exec()==QDialog::Accepted) {
    QString expr(c->result());
    expr.replace(_exponentiate,Expression::exponentiate());
    expr.replace(_multiply    ,Expression::multiply());
    expr.replace(_divide      ,Expression::divide());
    expr.replace(_add         ,Expression::add());
    expr.replace(_subtract    ,Expression::subtract());
    source_edit->setText(expr);
  }
  delete c;
}

void TdcClient::validate_source()
{
  //  Don't know how to validate yet
}

void TdcClient::validate_vsource()
{
  //  Don't know how to validate yet
}
