#ifndef AmiQt_Contour_hh
#define AmiQt_Contour_hh

#include "ami/qt/ImageMarker.hh"
#include <QtGui/QWidget>

#include "ami/data/Contour.hh"

class QLineEdit;

namespace Ami {
  namespace Qt {
    class ImageFrame;
    class RectangleCursors;

    class Contour : public QWidget, 
		    public ImageMarker {
    public:
      Contour(const char* x, const char* y,
	      const ImageFrame&,
	      const RectangleCursors&);
      ~Contour();
    public:
      void draw(QImage&);
      void setup(const char*,const char*);
      void save(char*& p) const;
      void load(const char*& p);
    public:
      Ami::Contour value() const;
    private:
      const ImageFrame&       _image;
      const RectangleCursors& _frame;
      QLineEdit* _c[Ami::Contour::MaxOrder+1];
      QLineEdit* _discrimLevelEdit;
      double     _discrimLevel;
    };
  };
};

#endif
