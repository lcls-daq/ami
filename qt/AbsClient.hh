#ifndef AmiQt_AbsClient_hh
#define AmiQt_AbsClient_hh


#include "ami/qt/Requestor.hh"
#include "ami/qt/QtTopWidget.hh"
#include "ami/client/AbsClient.hh"

namespace Ami {
  namespace Qt {
    class AbsClient : public QtTopWidget, 
		      public Ami::AbsClient,
		      public Requestor {
    public:
      AbsClient(QWidget*            parent,
		const Pds::DetInfo& src, 
		unsigned            channel) :
	QtTopWidget(parent,src,channel) {}
      virtual ~AbsClient() {}
    };
  };
};

#endif