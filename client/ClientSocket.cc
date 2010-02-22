#include "ami/client/ClientSocket.hh"
#include "ami/client/ClientManager.hh"

#include <stdio.h>

using namespace Ami;

ClientSocket::ClientSocket(ClientManager& mgr, int s) :
  TSocket(s),
  _mgr(mgr)
{
  Ins src = this->ins();
  printf("new ClientSocket bound to %x/%d\n",src.address(),src.portId());
  _mgr.add_client(*this);
}

ClientSocket::~ClientSocket() 
{
  _mgr.remove_client(*this);
}

int ClientSocket::fd() const { return _socket; }

int ClientSocket::processIo()
{
  _mgr.handle_client_io(*this);
  return 1;
}

int ClientSocket::processIo(const char* p,int l) 
{
  write((const void*)p,l); 
  return 1;
}

