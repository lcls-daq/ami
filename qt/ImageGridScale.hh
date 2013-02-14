#ifndef AmiQt_ImageGridScale_hh
#define AmiQt_ImageGridScale_hh

#include <QtGui/QGroupBox>

class QGridLayout;
class QButtonGroup;

#include <list>

namespace Ami {

  class Cds;

  namespace Qt {
    class ImageFrame;
    class CrossHair;
    //    class CrossHairDelta;

    class ImageGridScale : public QGroupBox {
      Q_OBJECT
    public:
      ImageGridScale(ImageFrame&, bool grab);
      ~ImageGridScale();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void update();
    public:
      ImageFrame& frame();
      void setup_payload(Cds&);
    public slots:
      void phy_scale(bool);
      virtual void setVisible(bool);
      void save_list();
      void load_list();
    private:
      ImageFrame& _frame;
      QGridLayout* _clayout;
      QButtonGroup* _group;
      unsigned _nrows;
      std::list<CrossHair*> _cross_hairs;
      //      CrossHairDelta* _delta;
      bool   _scaled;
      double _scalex,_scaley;
    };
  };
};

#endif
