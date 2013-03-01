#include "ami/qt/QtPlotSelector.hh"

#include "ami/data/DescEntry.hh"
#include "ami/qt/OverlayParent.hh"
#include "ami/qt/QtPlot.hh"

#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>

using namespace Ami::Qt;

QtPlotSelector::QtPlotSelector(QWidget&       parent,
                               OverlayParent& src,
                               DescEntry*     desc,
                               SharedData*    shared) :
  QDialog(&parent),
  _src   (src),
  _desc  (desc),
  _shared(shared)
{
  setWindowTitle("Select Plot Window");
  setAttribute(::Qt::WA_DeleteOnClose, true);

  const QStringList& plots = QtPlot::names(desc->type());
  if (plots.size()==0) {
    printf("QtPlotSelector no existing plots of type %d\n",desc->type());
    delete this;
    return;
  }

  if (plots.size()==1) {
    QtPlot* plot = QtPlot::lookup(plots.at(0));
    _src.add_overlay(_desc, plot, shared);
    delete this;
    return;
  }

  //
  //  Loop through QtPlot's
  //    Add each valid one to this list
  //
  QListWidget* plotList = new QListWidget;
  for(int i=0; i<plots.size(); i++) {
    plotList->addItem(plots.at(i));
  }

  QPushButton* grabB = new QPushButton("Grab");

  QVBoxLayout* l = new QVBoxLayout;
  l->addWidget(plotList);
  l->addWidget(grabB);
  setLayout(l);

  connect(plotList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(plot_selected(QListWidgetItem*)));
  connect(grabB, SIGNAL(clicked()), this, SLOT(grab_plot()));

  show();
}

QtPlotSelector::~QtPlotSelector()
{
  QtPlot::select(0);
}

void QtPlotSelector::plot_selected(QListWidgetItem* item)
{
  QtPlot* plot = QtPlot::lookup(item->text());
  if (plot) {
    _src.add_overlay(_desc, plot, _shared);
  }
  close();
}

void QtPlotSelector::grab_plot()
{
  QtPlot::select(this);
}

void QtPlotSelector::plot_selected(QtPlot* plot)
{
  _src.add_overlay(_desc, plot, _shared);
  close();
}

Ami::DescEntry::Type QtPlotSelector::type() const
{
  return _desc->type();
}
