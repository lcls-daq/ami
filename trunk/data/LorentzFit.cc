#include "ami/data/LorentzFit.hh"

#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"

#include <gsl/gsl_multifit_nlin.h>

#include <math.h>

static const double ErrMin = 5;     // Minimum value where errors are ~Gaussian
static const double epsAbs = 0;     // Absolute parameter change for convergence
static const double epsRel = 1.e-6; // Relative parameter change for convergence
static const int maxiter = 100;     // Maximum allowed iterations for convergence

struct data {
  size_t  n;
  double dn;
  const double* x;
  const double* y;
  const double* s;
};

static int lorentz_f(const gsl_vector * x, 
                     void * data, 
                     gsl_vector * f)
{
  double norm  = gsl_vector_get(x,0);
  double mu    = gsl_vector_get(x,1);
  double gamma = gsl_vector_get(x,2);

  double n     = norm/M_PI;
  double gamd2 = 0.5*gamma;
  double gamd2sq = gamd2*gamd2;

  size_t  _n = ((struct data*)data)->n;
  n *= ((struct data*)data)->dn;
  const double* _x = ((struct data*)data)->x;
  const double* _y = ((struct data*)data)->y;
  const double* _s = ((struct data*)data)->s;

  for(unsigned i=0; i<_n; i++) {
    double dx = _x[i] - mu;
    double den = dx*dx + gamd2sq;
    double dy = _s[i];
    if (dy>0) {
      double y  = n*gamd2/den;
      double g  = (y - _y[i])/dy;
      gsl_vector_set(f,i,g);
    }
    else
      gsl_vector_set(f,i,0);
  }

  return GSL_SUCCESS;
}

static int lorentz_df(const gsl_vector * x, 
                      void * data, 
                      gsl_matrix * J) 
{
  double norm  = gsl_vector_get(x,0);
  double mu    = gsl_vector_get(x,1);
  double gamma = gsl_vector_get(x,2);

  double n     = norm/M_PI;
  double gamd2 = 0.5*gamma;
  double gamd2sq = gamd2*gamd2;

  size_t  _n = ((struct data*)data)->n;
  n *= ((struct data*)data)->dn;
  const double* _x = ((struct data*)data)->x;
  const double* _s = ((struct data*)data)->s;

  for(unsigned i=0; i<_n; i++) {
    double dx = _x[i] - mu;
    double den = dx*dx + gamd2sq;
    double dy = _s[i];
    if (dy>0) {
      double y  = n*gamd2/den;
      gsl_matrix_set(J,i,0,y/(norm*dy));
      gsl_matrix_set(J,i,1,(2*dx/den)*y/dy);
      gsl_matrix_set(J,i,2,(y/gamma - 2*y*y/n)/dy);
    }
    else {
      gsl_matrix_set(J,i,0,0);
      gsl_matrix_set(J,i,1,0);
      gsl_matrix_set(J,i,2,0);
    }
  }

  return GSL_SUCCESS;
}

static int lorentz_fdf(const gsl_vector * x, 
                       void * data, 
                       gsl_vector * f, 
                       gsl_matrix * J) 
{
  double norm  = gsl_vector_get(x,0);
  double mu    = gsl_vector_get(x,1);
  double gamma = gsl_vector_get(x,2);

  double n     = norm/M_PI;
  double gamd2 = 0.5*gamma;
  double gamd2sq = gamd2*gamd2;

  size_t  _n = ((struct data*)data)->n;
  n *= ((struct data*)data)->dn;
  const double* _x = ((struct data*)data)->x;
  const double* _y = ((struct data*)data)->y;
  const double* _s = ((struct data*)data)->s;

  for(unsigned i=0; i<_n; i++) {
    double dx = _x[i] - mu;
    double den = dx*dx + gamd2sq;
    double dy = _s[i];
    if (dy>0) {
      double y  = n*gamd2/den;
      double g  = (y - _y[i])/dy;
      gsl_vector_set(f,i,g);
      gsl_matrix_set(J,i,0,y/(norm*dy));
      gsl_matrix_set(J,i,1,(2*dx/den)*y/dy);
      gsl_matrix_set(J,i,2,(y/gamma - 2*y*y/n)/dy);
    }
    else {
      gsl_vector_set(f,i,0);
      gsl_matrix_set(J,i,0,0);
      gsl_matrix_set(J,i,1,0);
      gsl_matrix_set(J,i,2,0);
    }
  }
  return GSL_SUCCESS;
}

namespace Ami {
  class Stat {
  public:
    Stat() : _n(0), _x(0), _xx(0) {}
  public:
    void accum(double x, double w) { _n+=w; _x+=w*x, _xx+=w*x*x; }
  public:
    double n   () const { return _n; }
    double mean() const { return _x/_n; }
    double rms () const { return sqrt(_xx/_n - pow(mean(),2)); }
  private:
    double _n,_x,_xx;
  };
};

using namespace Ami;

LorentzFit::LorentzFit() : _params(3), _converged(false)
{
}

LorentzFit::~LorentzFit()
{
}

void LorentzFit::fit(const EntryTH1F& e, double xlo, double xhi)
{
  Stat s;

  std::vector<double> vx;
  std::vector<double> vy;
  std::vector<double> vs;

  const DescTH1F& desc = e.desc();
  if (xlo==0 && xhi==0) {
    xlo = desc.xlow();
    xhi = desc.xup ();
  }
  double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
  double xo = desc.xlow()+0.5*dx;
  for(unsigned i=0; i<desc.nbins(); i++) {
    double x = xo+double(i)*dx;
    if (x < xlo || x > xhi) continue;
    double y = e.content(i);
    if (y > ErrMin) {
      vx.push_back(x);
      vy.push_back(y);
      s.accum(x,y);
      vs.push_back(sqrt(y));
    }
  }

  //  if (!_converged) {
  {
    _params[0] = s.n();
    _params[1] = s.mean();
    _params[2] = s.rms()*2.35;
  }

  _fit(vx,vy,vs,dx);

  double gamd2 = 0.5*_params[2];
  double gamd2sq = gamd2*gamd2;
  double a = dx*(_params[0]/M_PI)*gamd2;
  if (e.desc().isnormalized())
    a /= e.info(EntryTH1F::Normalization);

  if (vx.size()<2) return;

  unsigned n = 2*unsigned((vx.back()-vx.front())/dx+1.5)+1;
  _x.resize(n);
  _y.resize(n);
  dx *= 0.5;
  xo = vx.front()-dx;
  for(unsigned i=0; i<n; i++) {
    double x = xo+double(i)*dx;
    _x[i] = x;
    x = x - _params[1];
    _y[i] = a/(x*x + gamd2sq);
  }
}

void LorentzFit::fit(const EntryProf& e,double xlo,double xhi)
{
  Stat s;

  std::vector<double> vx;
  std::vector<double> vy;
  std::vector<double> vs;

  const DescProf& desc = e.desc();
  if (xlo==0 && xhi==0) {
    xlo = desc.xlow();
    xhi = desc.xup ();
  }
  double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
  double xo = desc.xlow()+0.5*dx;
  for(unsigned i=0; i<desc.nbins(); i++) {
    double x = xo+double(i)*dx;
    if (x < xlo || x > xhi) continue;
    double y = e.ymean(i);
    vx.push_back(x);
    vy.push_back(y);
    double n = e.entries()[i];
    vs.push_back(n>0 ? e.sigma(i)/sqrt(n) : 0);
    s.accum(x,y);
  }

  //  if (!_converged) {
  {
    _params[0] = s.n();
    _params[1] = s.mean();
    _params[2] = s.rms()*2.35;
  }

  _fit(vx,vy,vs,1.);

  double gamd2 = 0.5*_params[2];
  double gamd2sq = gamd2*gamd2;
  double a = (_params[0]/M_PI)*gamd2;

  if (vx.size()<2) return;

  unsigned n = 2*unsigned((vx.back()-vx.front())/dx+1.5)+1;

  _x.resize(n);
  _y.resize(n);
  dx *= 0.5;
  xo  = vx.front()-dx;
  for(unsigned i=0; i<n; i++) {
    double x = xo+double(i)*dx;
    _x[i] = x;
    x = x - _params[1];
    _y[i] = a/(x*x + gamd2sq);
  }
}

std::vector<double>  LorentzFit::params() const { return _converged ? _params : std::vector<double>(0); }
std::vector<QString> LorentzFit::names () const {
  std::vector<QString> n;
  n.push_back(QString("A"));
  n.push_back(QString(QChar(0x3bc)));   // mu
  n.push_back(QString(QChar(0x393)));  // Gamma
  return n;
}
const std::vector<double>&  LorentzFit::x() const { return _x; }
const std::vector<double>&  LorentzFit::y() const { return _y; }

void LorentzFit::_fit(const std::vector<double>& vx,
                      const std::vector<double>& vy,
                      const std::vector<double>& vs,
                      double dn)
{ 
  unsigned n = vx.size();
  unsigned p = _params.size();

  if (n<p) { _converged=false; return; }

  struct data d;
  d.n = n;
  d.dn= dn;
  d.x = vx.data();
  d.y = vy.data();
  d.s = vs.data();

  const gsl_multifit_fdfsolver_type * T = gsl_multifit_fdfsolver_lmsder;
  gsl_multifit_fdfsolver* solver = gsl_multifit_fdfsolver_alloc(T,n,p);

  gsl_multifit_function_fdf f;
  f.f   = &lorentz_f;
  f.df  = &lorentz_df;
  f.fdf = &lorentz_fdf;
  f.n   = n;
  f.p   = p;
  f.params = &d;

  gsl_vector_view gp = gsl_vector_view_array (_params.data(), p);
  gsl_multifit_fdfsolver_set(solver,&f,&gp.vector);

  int iter, status;
  for(iter=0,status=GSL_CONTINUE;
      status==GSL_CONTINUE && iter<maxiter; iter++) {

    status = gsl_multifit_fdfsolver_iterate (solver);

    if (status)
      break;

    status = gsl_multifit_test_delta (solver->dx, solver->x,
                                      epsAbs, epsRel);
  }

  _converged = (status==GSL_SUCCESS);

  for(unsigned i=0; i<p; i++)
    _params[i] = gsl_vector_get(solver->x, i);

  gsl_multifit_fdfsolver_free(solver);
}
