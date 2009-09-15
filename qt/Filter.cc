#include "Filter.hh"

#include "Condition.hh"
#include "CExpression.hh"

#include "ami/data/RawFilter.hh"
#include "ami/data/FeatureRange.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/Calculator.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QComboBox>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtGui/QRegExpValidator>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

#include <fstream>
using std::ifstream;
using std::ofstream;

namespace Ami {
  namespace Qt {
  };
};

using namespace Ami::Qt;

Filter::Filter(const QString& title) :
  QWidget   (0),
  _name     (title),
  _expr     (new QLineEdit),
  _cond_name(new QLineEdit("A")),
  _features (new QComboBox),
  _lo_rng   (new QLineEdit("0")),
  _hi_rng   (new QLineEdit("1")),
  _clayout  (new QVBoxLayout),
  _filter   (new RawFilter)
{
  setWindowTitle(title);
  setAttribute(::Qt::WA_DeleteOnClose, false);

  _cond_name->setMaximumWidth(60);
  _lo_rng   ->setMaximumWidth(60);
  _hi_rng   ->setMaximumWidth(60);
  new QRegExpValidator(QRegExp("[a-zA-Z]+"),_cond_name);
  new QDoubleValidator(_lo_rng);
  new QDoubleValidator(_hi_rng);

  QPushButton* addB   = new QPushButton("Add");
  QPushButton* calcB  = new QPushButton("Enter");
  QPushButton* applyB = new QPushButton("Apply");
  QPushButton* clearB = new QPushButton("Clear");
  QPushButton* okB    = new QPushButton("OK");
  QPushButton* cancelB= new QPushButton("Cancel");
  QPushButton* loadB  = new QPushButton("Load");
  QPushButton* saveB  = new QPushButton("Save");

  QHBoxLayout* l = new QHBoxLayout;
  QVBoxLayout* layout = new QVBoxLayout;
  { QGroupBox* file_box = new QGroupBox("File");
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(loadB);
    layout1->addWidget(saveB);
    layout1->addStretch();
    file_box->setLayout(layout1);
    layout->addWidget(file_box); }
  { QGroupBox* conditions_box = new QGroupBox("Define Conditions");
    conditions_box->setToolTip("A CONDITION is an inclusive range of one of the predefined observables." \
			       "The CONDITION is defined by expression " \
			       "[name] := [lo value] <= [observable] <= [hi value]" );
    QVBoxLayout* layout2 = _clayout;
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addWidget(_cond_name);
      layout1->addWidget(new QLabel(" := "));
      layout1->addWidget(_lo_rng);
      layout1->addWidget(new QLabel("<="));
      //      layout1->addWidget(_bld_box);
      layout1->addWidget(_features);
      layout1->addWidget(new QLabel("<="));
      layout1->addWidget(_hi_rng);
      layout1->addStretch();
      layout1->addWidget(addB);
      layout2->addLayout(layout1); }
    conditions_box->setLayout(layout2);
    layout->addWidget(conditions_box); }
  { QGroupBox* expr_box = new QGroupBox(QString("Expression"));
    expr_box->setToolTip(QString("An EXPRESSION is a set of CONDITIONS separated by the operators\n " \
				 "  A %1 B : logical AND of A and B \n"	\
				 "  A %2 B : logical OR of A and B \n").arg(CExpression::logicAnd()).arg(CExpression::logicOr()));
    QVBoxLayout* layout2 = new QVBoxLayout;
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addWidget(_expr); _expr->setReadOnly(true);
      layout1->addWidget(calcB);
      layout2->addLayout(layout1); }
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addStretch();
      layout1->addWidget(applyB);
      layout1->addWidget(clearB);
      layout1->addWidget(okB);
      layout1->addWidget(cancelB);
      layout1->addStretch();
      layout2->addLayout(layout1); }
    expr_box->setLayout(layout2);
    layout->addWidget(expr_box); }
  l->addLayout(layout);
  l->addStretch();
  setLayout(l);

  connect(addB  , SIGNAL(clicked()), this, SLOT(add()));
  connect(calcB , SIGNAL(clicked()), this, SLOT(calc()));
  connect(applyB, SIGNAL(clicked()), this, SLOT(apply()));
  connect(clearB, SIGNAL(clicked()), this, SLOT(clear()));
  connect(loadB , SIGNAL(clicked()), this, SLOT(load()));
  connect(saveB , SIGNAL(clicked()), this, SLOT(save()));
  connect(okB   , SIGNAL(clicked()), this, SLOT(apply()));
  connect(okB   , SIGNAL(clicked()), this, SLOT(hide()));
  connect(cancelB, SIGNAL(clicked()), this, SLOT(hide()));
  connect(&FeatureRegistry::instance(), SIGNAL(changed()),
	  this  , SLOT(update_features()));
}

Filter::~Filter()
{
  if (_filter) delete _filter;
}

void Filter::add  ()
{
  for(std::list<Condition*>::const_iterator it=_conditions.begin(); it!=_conditions.end(); it++)
    if ((*it)->name()==_cond_name->text()) {
      QMessageBox::critical(this, "Define Condition",
			    QString("Condition name %1 is not unique.").arg(_cond_name->text()));
      return;
    }

  Condition* c  = new Condition(_cond_name->text(),
				QString("%1 := %2 <= %3 <= %4")
				.arg(_cond_name->text())
				.arg(_lo_rng->text())
				.arg(_features->currentText())
				.arg(_hi_rng->text()),
				new FeatureRange(_features->currentIndex(),
						 _lo_rng->text().toDouble(),
						 _hi_rng->text().toDouble()));
  _conditions.push_back(c);
  _clayout->addWidget(c);

  connect(c, SIGNAL(removed(const QString&)), this, SLOT(remove(const QString&)));
}

void Filter::remove(const QString& name)
{

  for(std::list<Condition*>::iterator it=_conditions.begin(); it!=_conditions.end(); it++)
    if ((*it)->name() == name) {
      delete (*it);
      _conditions.remove(*it);
      break;
    }
}

void Filter::apply()
{
  if (_filter) delete _filter;

  CExpression parser(_conditions);
  _filter = parser.evaluate(_expr->text());

  if (_filter==0) {
    printf("Filter parser failed to evaluate %s\nResetting filter.\n",qPrintable(_expr->text()));
    clear();
  }

  emit changed();
}

void Filter::clear()
{
  _expr->clear();

  if (_filter) delete _filter;
  _filter = new RawFilter;
}

void Filter::load()
{
  QString file = QFileDialog::getOpenFileName(this,"File to read from:",
					      "", "(filter) *.flt;; (all) *");
  if (file.isNull())
    return;

  ifstream f(qPrintable(file));
  if (!f.good())
    QMessageBox::warning(this, "Load",
			 QString("Error opening %1 for reading").arg(file));
  else {
    for(std::list<Condition*>::const_iterator it=_conditions.begin(); it!=_conditions.end(); it++)
      delete *it;
    _conditions.clear();

    char buffer[256];
    f.getline(buffer,256);
    _expr->setText(buffer); 

    char condition[64];
    char variable [64];
    double lo, hi;
    f.getline(buffer,256);
    while(f.good()) {
      sscanf(buffer,"%s := %lg <= %s <= %lg", condition, &lo, variable, &hi);
      printf("new condition %s := %g <= %s <= %g\n",
	     condition, lo, variable, hi);
      int index = _features->findText(variable);
      if (index<0)
	printf("Unable to identify %s\n",variable);
      else {
	Condition* c = new Condition(condition,
				     QString("%1 := %2 <= %3 <= %4")
				     .arg(condition)
				     .arg(lo)
				     .arg(variable)
				     .arg(hi),
				     new FeatureRange(index,lo,hi));
	_conditions.push_back(c);
	_clayout->addWidget(c);
	connect(c, SIGNAL(removed(const QString&)), this, SLOT(remove(const QString&)));
      }
      f.getline(buffer,256);
    }
  }
}

void Filter::save()
{
  char time_buffer[32];
  time_t seq_tm = time(NULL);
  strftime(time_buffer,32,"%Y%m%d_%H%M%S",localtime(&seq_tm));

  QString def(_name);
  def += "_";
  def += time_buffer;
  def += ".flt";
  QString fname =
    QFileDialog::getSaveFileName(this,"Save File As (.flt)",
				 def,".flt");
  if (!fname.isNull()) {
    ofstream f(qPrintable(fname));
    if (!f.good())
      QMessageBox::warning(this, "Save data",
			   QString("Error opening %1 for writing").arg(fname));
    else {
      f << qPrintable(_expr->text()) << std::endl;
      for(std::list<Condition*>::const_iterator it=_conditions.begin(); it!=_conditions.end(); it++)
	f << qPrintable((*it)->label()) << std::endl;
    }
  }
}

void Filter::update_features()
{
  _features->clear();
  _features->addItems(FeatureRegistry::instance().names());
}

const Ami::AbsFilter* Filter::filter() const { return _filter; }

void Filter::calc()
{
  QStringList conditions;
  for(std::list<Condition*>::const_iterator it=_conditions.begin();
      it != _conditions.end(); it++)
    conditions << (*it)->name();

  QStringList ops;
  ops << CExpression::logicAnd() << CExpression::logicOr();
  
  QStringList none;

  Calculator* c = new Calculator(tr("Filter Expression"),"",
 				 conditions, ops, none);
  if (c->exec()==QDialog::Accepted)
    _expr->setText(c->result());

   delete c;
}
