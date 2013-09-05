#include "ami/qt/SMPWarning.hh"
#include "ami/qt/SMPRegistry.hh"

#include <QtGui/QMessageBox>

using namespace Ami::Qt;

SMPWarning::SMPWarning() :
  QPushButton("Display\nOnly")
{
  setPalette(QPalette(::Qt::yellow));

  connect(this, SIGNAL(clicked()), this, SLOT(showWarning()));
  connect(&SMPRegistry::instance(), SIGNAL(changed()), this, SLOT(updateVisibility()));

  updateVisibility();
}

SMPWarning::~SMPWarning() {}

void SMPWarning::updateVisibility()
{
  setVisible(SMPRegistry::instance().nservers()>1);
}

void SMPWarning::showWarning() 
{
  QString title("Distributed Processing Environment");
  QString text ("These operations can only produce final displays.  No derived processing will be available, though the equivalent operation can be found elsewhere.");
  QMessageBox::information(this,title,text);
}

