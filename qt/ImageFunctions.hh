#ifndef AmiQt_ImageFunctions_hh
#define AmiQt_ImageFunctions_hh

#include <QtGui/QWidget>

class QComboBox;
class QLineEdit;

namespace Ami {
  namespace Qt {
    class ScalarPlotDesc;
    class ImageFunctions : public QWidget {
      Q_OBJECT
    public:
      ImageFunctions(QWidget* parent);
      ~ImageFunctions();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      ScalarPlotDesc& plot_desc() { return *_plot_desc; }
      const char* expression() const;
    public:
      void update_range (int x1, int y1, int x2, int y2);
      void update_range (double xc, double yc,
                         double r1, double r2,
                         double f0, double f1);
    public slots:
      void update_function(int);
    private:
      QComboBox*      _functions;
      ScalarPlotDesc* _plot_desc;
      QLineEdit*      _expr_edit;
      QString         _expr;
    };
  };
};

#endif
