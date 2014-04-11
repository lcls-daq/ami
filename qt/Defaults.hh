#ifndef AmiQt_Defaults_hh
#define AmiQt_Defaults_hh

#include "ami/qt/QtPWidget.hh"

class QCheckBox;
class QComboBox;
class QLineEdit;

namespace Ami {
  namespace Qt {
    class Defaults : public QtPWidget {
    public:
      bool select_run     () const;
      bool show_grid      () const;
      bool show_minor_grid() const;
      double image_update_rate() const;
      double other_update_rate() const;
      int plot_width () const;
      int plot_height() const;
      QString movie_format() const;
      int  save_precision () const;
    public:
      static Defaults* instance();
    private:
      Defaults();
      ~Defaults();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    private:
      QCheckBox* _run;
      QCheckBox* _grid;
      QCheckBox* _minor_grid;
      QLineEdit* _image_rate;
      QLineEdit* _others_rate;
      QLineEdit* _plot_width;
      QLineEdit* _plot_height;
      QComboBox* _movie_format_box;
      QComboBox* _save_precision;
    };
  };
};

#endif
