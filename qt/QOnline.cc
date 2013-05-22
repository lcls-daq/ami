#include "ami/qt/QOnline.hh"
#include "ami/service/Fd.hh"
#include "ami/service/TSocket.hh"
#include "ami/service/Ins.hh"

#include "pdsapp/monobs/MonShmComm.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>

#include <netdb.h>
#include <sys/socket.h>

//
//  Communications with monshmservers:
//    (1) get/set node mask
//    (2) get statistics { Events, EB failures }
//
namespace Ami {
  namespace Qt {
    class NodeInfo {
    public:
      std::string name;
      int         ip;
      unsigned    groups;
      unsigned    mask;
      unsigned    rmask;
      unsigned    events;
      unsigned    revents;
      unsigned    dmg;
      unsigned    rdmg;
    };

    class ShmServer : public Fd {
    public:
      ShmServer(QOnline& qo, QString& qnode) : _qo(qo) 
      {
	Ami::Ins ins(Ami::Ins::parse_ip(qPrintable(qnode)),
		     Pds::MonShmComm::ServerPort);
	while(1) {
	  try {
	    _socket.connect(ins);
	  }
	  catch(Event& e) {
	    sleep(1);
	    continue;
	  }
	  break;
	}

	Pds::MonShmComm::Get get;
	_socket.read(&get, sizeof(get));

	_info.name   = std::string(get.hostname);
	_info.ip     = ins.address();
	_info.groups = get.groups;	
	_info.mask   = get.mask;
	_info.rmask  = get.mask;
	_info.events = get.events;
	_info.revents= get.events;
	_info.dmg    = get.dmg;
	_info.rdmg   = get.dmg;
      }
    public:
      int fd() const { return _socket.socket(); }
      int processIo() 
      { 
	Pds::MonShmComm::Get m;
	if (_socket.read(&m,sizeof(m))==sizeof(m)) {
	  _info.events = _info.revents;
	  _info.dmg    = _info.rdmg;
	  _info.rmask  = m.mask;
	  _info.revents= m.events;
	  _info.rdmg   = m.dmg;
	  _qo.update();
	  return 1;
	}
	else {
	  _qo.remove(this);
	  return 0;
	}
      }
    public:
      void set_node_mask(unsigned mask) 
      { Pds::MonShmComm::Set m(mask);
	_socket.write(&m,sizeof(m)); }
      NodeInfo&       info()       { return _info; }
      const NodeInfo& info() const { return _info; }
    private:
      QOnline& _qo;
      TSocket _socket;
      NodeInfo _info;
    };
  }
}

Ami::Qt::QOnline::QOnline(const char* nodes) :
  QGroupBox("Sources"),
  Poll(1000)
{
  if (nodes) {
    QStringList qnodes;

    char* node = strtok(const_cast<char*>(nodes),",");
    while(node) {
      QString qnode(node);
      //    qnode.strip(" ");
      qnodes << qnode;

      ShmServer* s = new ShmServer(*this,qnode);
      _servers.push_back(s);
      manage(*s);

      node = strtok(NULL,",");
    }

    QHBoxLayout* hl = new QHBoxLayout;
    _applyB = new QPushButton("Apply");
    _applyB->setEnabled(false);
    connect(_applyB, SIGNAL(clicked()), this, SLOT(apply_mask()));
  
    QPushButton* exitB = new QPushButton("Exit");
    connect(exitB, SIGNAL(clicked()), this, SIGNAL(exit()));
  
    hl->addWidget(_applyB);
    hl->addStretch();
    hl->addWidget(exitB);
  
    QVBoxLayout* layout = new QVBoxLayout;
    layout->addLayout(hl);
    layout->addLayout(_layout = new QGridLayout);

    QGridLayout* l = new QGridLayout;

    _rows.clear();

    int row = 0;
  
    _mask_group = new QButtonGroup;
    _mask_group->setExclusive(false);
  
    { // create title row
      const ShmServer& s = *_servers[0];
      int col=0;
      l->addWidget(new QLabel("Mon Node"), row, col++,1,1,::Qt::AlignBottom);
      l->addWidget(new QLabel("Dss Nodes"), row, col, 1, s.info().groups, ::Qt::AlignHCenter);
      col += s.info().groups;
      //    for(unsigned j=0; j<s.info().groups; j++)
      //      l->addWidget(new Ami::Qt::QrLabel(QString("DSS0%1").arg(j+1)),row, col++,1,1,::Qt::AlignHCenter);
      l->addWidget(new QLabel("Evt"),row,col++,1,1,::Qt::AlignBottom);
      l->addWidget(new QLabel("Dmg"),row,col++,1,1,::Qt::AlignBottom);
      row++;
    }

    for(unsigned i=0; i<_servers.size(); i++) {
      const ShmServer& s = *_servers[i];
    
      //  Add node row
      RowWidgets qrow;
      int col=0;
      l->addWidget(new QLabel(s.info().name.c_str()), row, col++);
      //    for(unsigned j=0; j<s.info().groups; j++) {
      for(unsigned j=0; j<s.info().groups; j++) {
	QCheckBox* box = new QCheckBox;
	l->addWidget(box, row, col++);
	qrow._box.push_back(box);
	box->setChecked(s.info().mask & (1<<j));
	_mask_group->addButton(box,i*10+j);
      }
      l->addWidget(qrow._events=new QLabel, row, col++);
      l->addWidget(qrow._dmg   =new QLabel, row, col++);
      _rows.push_back(qrow);
    
      row++;
    }

    connect(_mask_group, SIGNAL(buttonClicked(int)), this, SLOT(update_mask(int)));
    layout->addLayout(l);

    setLayout(layout);
    connect(this, SIGNAL(updated()), this, SLOT(_update()));
    connect(this, SIGNAL(removed(int)), this, SLOT(_remove(int)));

    start();
  }
  else {
    QPushButton* exitB = new QPushButton("Exit");
    connect(exitB, SIGNAL(clicked()), this, SIGNAL(exit()));
    QHBoxLayout* hl = new QHBoxLayout;
    hl->addWidget(exitB);
    setLayout(hl);
  }
}

Ami::Qt::QOnline::~QOnline()
{
}

void Ami::Qt::QOnline::update()
{
  emit updated();
}

void Ami::Qt::QOnline::remove(ShmServer* s)
{
  for(unsigned i=0; i<_servers.size(); i++)
    if (_servers[i]==s) {
      emit removed(i);
      break;
    }
}

int Ami::Qt::QOnline::processTmo() 
{
  return 1;
}

void Ami::Qt::QOnline::_update()
{
  // loop over nfds and update display
  bool lmask=false;
  for(unsigned i=0; i<_servers.size(); i++) {
    if (!_servers[i]) continue;
    const NodeInfo& info = _servers[i]->info();
    lmask |= (info.mask!=info.rmask);
    for(unsigned j=0; j<info.groups; j++)
      _rows[i]._box[j]->setChecked(info.mask & (1<<j));

    _rows[i]._events->setText(QString::number(info.revents-info.events));
    if (info.rdmg!=info.dmg) {
      _rows[i]._dmg   ->setPalette(QPalette(::Qt::yellow));
      _rows[i]._dmg   ->setText(QString::number(info.rdmg-info.dmg));
    }
    else {
      _rows[i]._dmg   ->setPalette(QPalette());
      _rows[i]._dmg   ->setText("0");
    }
  }
  _applyB->setEnabled(lmask);
}

void Ami::Qt::QOnline::_remove(int s)
{
  _servers[s] = 0;
  for(unsigned i=0; i<_rows[s]._box.size(); i++) {
    _rows[s]._box[i]->setChecked(false);
    _rows[s]._box[i]->setEnabled(false);
  }
  _rows[s]._events->setText("-");
  _rows[s]._dmg   ->setText("-");
}

void Ami::Qt::QOnline::update_mask(int b)
{
  int row = b/10;
  int col = b%10;
  for(int i=0; i<int(_rows.size()); i++) {
    if (!_servers[i]) continue;
    if (i!=row) {
      _servers[i]->info().mask &= ~(1<<col);
      _rows[i]._box[col]->setChecked(false);
    }
    else {
      _servers[i]->info().mask ^= (1<<col);
    }
  }
}

void Ami::Qt::QOnline::apply_mask()
{
  for(unsigned i=0; i<_rows.size(); i++) {
    if (!_servers[i]) continue;
    unsigned mask=0;
    for(unsigned j=0; j<_rows[i]._box.size(); j++)
      if (_rows[i]._box[j]->isChecked())
	mask |= (1<<j);
    _servers[i]->set_node_mask(mask);
  }
}
