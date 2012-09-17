#ifndef AmiQt_ImageXYControl_hh
#define AmiQt_ImageXYControl_hh

#include <QtGui/QGroupBox>
#include <QtCore/QString>

namespace Ami {
  namespace Qt {
    class ImageXYControl : public QGroupBox {
      Q_OBJECT
    public:
      ImageXYControl(QWidget*, const QString& title);
      ~ImageXYControl();
    public:
      float  scale   () const;
    public slots:
      void reset();
      void zoom();
      void pan ();
    signals:
      void windowChanged();
    private:
      int     _scale;
    };
  };
};

#endif
