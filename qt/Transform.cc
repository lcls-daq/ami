#include "Transform.hh"

#include "TransformConstant.hh"

#include "ami/qt/Calculator.hh"
#include "ami/data/Expression.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

#include <fstream>
using std::ifstream;
using std::ofstream;

namespace Ami {
  namespace Qt {
    class TransformAxis : public Variable {
    public:
      TransformAxis(const QString& name, double& v) : _name(name), _v(v) {}
      ~TransformAxis() {}
    public:
      Variable*      clone   () const { return new TransformAxis(_name,_v); }
      const QString& name    () const { return _name; }
      double         evaluate() const { return _v; }
    private:
      QString _name;
      double& _v;
    };
  };
};

using namespace Ami::Qt;

static QChar _exponentiate(0x005E);
static QChar _multiply    (0x00D7);
static QChar _divide      (0x00F7);
static QChar _add         (0x002B);
static QChar _subtract    (0x002D);

Transform::Transform(const QString& title,
		     const QString& axis) :
  QWidget   (0),
  _name     (axis),
  _expr     (new QLineEdit),
  _new_name (new QLineEdit),
  _new_value(new QLineEdit),
  _clayout  (new QVBoxLayout)
{
  setWindowTitle(title);
  setAttribute(::Qt::WA_DeleteOnClose, false);

  //  _expr->setFrameShape(QFrame::Box);
  //  _expr->setWordWrap(true);
  _expr->setText(_name);
  _expr->setReadOnly(true);
  _term = 0;

  new QDoubleValidator(_new_value);

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
  { QGroupBox* constants_box = new QGroupBox("Define Constants");
    constants_box->setToolTip("Assign a _named_ variable with a constant _value_.");
    QVBoxLayout* layout2 = _clayout;
    { QGridLayout* layout1 = new QGridLayout;
      layout1->addWidget(new QLabel("Name"),0,0);
      layout1->addWidget(new QLabel("Value"),1,0);
      layout1->addWidget(_new_name ,0,1);
      layout1->addWidget(_new_value,1,1);
      layout1->setColumnStretch(2,1);
      layout1->addWidget(addB,0,3,2,1,::Qt::AlignVCenter);
      layout2->addLayout(layout1); }
    constants_box->setLayout(layout2);
    layout->addWidget(constants_box); }
  { QGroupBox* expr_box = new QGroupBox("Expression:");
    expr_box->setToolTip(QString("Transform the axis coordinate x into x' using named constants and the following operators\n" \
				 "  A %1 B  : A to the power of B\n"\
				 "  A %2 B  : A multiplied by B\n"\
				 "  A %3 B  : A divided by B\n"\
				 "  A %4 B  : A added to B\n"\
				 "  A %5 B  : A subtracted by B\n"\
				 "  where A and B can be a named constant or the axis coordinate name.")
			 .arg(_exponentiate)
			 .arg(_multiply)
			 .arg(_divide)
			 .arg(_add)
			 .arg(_subtract));
    QVBoxLayout* layout2 = new QVBoxLayout;
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addWidget(new QLabel(_name+QString("\' = ")));
      layout1->addWidget(_expr);
      layout1->addWidget(calcB);
      layout2->addLayout(layout1); }
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addWidget(applyB);
      layout1->addWidget(clearB);
      layout1->addWidget(okB);
      layout1->addWidget(cancelB);
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
}

Transform::~Transform()
{
  if (_term) delete _term;
}

void Transform::add  ()
{
  TransformConstant* c  = new TransformConstant(_new_name->text(),_new_value->text().toDouble());
  _constants.push_back(c);
  _clayout->addWidget(c);

  connect(c, SIGNAL(removed(const QString&)), this, SLOT(remove(const QString&)));
}

void Transform::remove(const QString& name)
{
  for(std::list<TransformConstant*>::iterator it=_constants.begin(); it!=_constants.end(); it++)
    if ((*it)->name() == name) {
      delete (*it);
      _constants.remove(*it);
      break;
    }
}

void Transform::calc()
{
  QStringList variables;
  variables << _name;
  for(std::list<TransformConstant*>::const_iterator it=_constants.begin(); it!=_constants.end(); it++)
    variables << (*it)->name();

  QStringList ops;
  ops << _exponentiate
      << _multiply
      << _divide  
      << _add     
      << _subtract;

  QStringList none;

  Calculator* c = new Calculator(tr("Y Transform"),_name,
 				 variables, none, ops);
  if (c->exec()==QDialog::Accepted)
    _expr->setText(c->result());

   delete c;
}

void Transform::apply()
{
  std::list<Variable*> variables;
  variables.push_back(new TransformAxis(_name,_axis));
  for(std::list<TransformConstant*>::const_iterator it=_constants.begin(); it!=_constants.end(); it++)
    variables.push_back(new Constant((*it)->name(), (*it)->value()));

  if (_term) delete _term;

  Expression parser(variables);
  QString expr = _expr->text();
  expr.replace(_exponentiate,Expression::exponentiate());
  expr.replace(_multiply    ,Expression::multiply());
  expr.replace(_divide      ,Expression::divide());
  expr.replace(_add         ,Expression::add());
  expr.replace(_subtract    ,Expression::subtract());
  _term = parser.evaluate(expr);
  if (_term==0) {
    QMessageBox::critical(this, QString("Evaluate transform"), QString("Unable to parse expression."));
  }

  for(std::list<Variable*>::iterator it=variables.begin(); it!=variables.end(); it++)
    delete (*it);

  emit changed();
}

void Transform::clear()
{
  _expr->setText(_name);
}

void Transform::load()
{
  QString file = QFileDialog::getOpenFileName(this,"File to read from:",
					      "", "(axis) *.axis;; (all) *");
  if (file.isNull())
    return;

  ifstream f(qPrintable(file));
  if (!f.good())
    QMessageBox::warning(this, "Load",
			 QString("Error opening %1 for reading").arg(file));
  else {
    for(std::list<TransformConstant*>::const_iterator it=_constants.begin(); it!=_constants.end(); it++)
      delete *it;
    _constants.clear();

    char buffer[256];
    f.getline(buffer,256);
    _expr->setText(buffer);

    double v;
    f >> buffer >> buffer[255] >> v;
    while(f.good()) {
      printf("new constant %s = %g\n",buffer,v);
      TransformConstant* c = new TransformConstant(buffer,v);
      _constants.push_back(c);
      _clayout->addWidget(c);
      connect(c, SIGNAL(removed(const QString&)), this, SLOT(remove(const QString&)));
      f >> buffer >> buffer[255] >> v;
    }
  }
}

void Transform::save()
{
  char time_buffer[32];
  time_t seq_tm = time(NULL);
  strftime(time_buffer,32,"%Y%m%d_%H%M%S",localtime(&seq_tm));

  QString def(_name);
  def += "_";
  def += time_buffer;
  def += ".axis";
  QString fname =
    QFileDialog::getSaveFileName(this,"Save File As (.axis)",
				 def,".axis");
  if (!fname.isNull()) {
    ofstream f(qPrintable(fname));
    if (!f.good())
      QMessageBox::warning(this, "Save data",
			   QString("Error opening %1 for writing").arg(fname));
    else {
      f << qPrintable(_expr->text()) << std::endl;
      for(std::list<TransformConstant*>::const_iterator it=_constants.begin(); it!=_constants.end(); it++)
	f << qPrintable((*it)->name()) << " = " << (*it)->value() << std::endl;
    }
  }
}

double Transform::operator()(double u) const
{
  if (_term) {
    _axis=u;
    return _term->evaluate();
  }
  else return u;
}
