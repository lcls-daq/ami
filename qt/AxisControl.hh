#ifndef AmiQt_AxisControl_hh
#define AmiQt_AxisControl_hh

#include <QtGui/QGroupBox>

#include <QtCore/QString>

class QLineEdit;
class QScrollBar;
class QPushButton;

//#define USE_SCROLL

namespace Ami {
  namespace Qt {
    class AxisInfo;
    class AxisControl : public QGroupBox {
      Q_OBJECT
    public:
      AxisControl(QWidget*, const QString& title);
      ~AxisControl();
    public:
      void update(const AxisInfo&);
    public:
      bool   isAuto() const;
      double loEdge() const;
      double hiEdge() const;
    public slots:
      void changeLoEdge(const QString&);
      void changeHiEdge(const QString&);
      void auto_scale(bool);
      void updateInfo();
#ifdef USE_SCROLL
      void changeLoEdge(int);
      void changeWindow(int,int);
      void zoom();
      void pan ();
    signals:
      void windowChanged(int,int);
#else
    signals:
      void windowChanged();
#endif
    private:
      QString _title;
      const AxisInfo* _info;
      QScrollBar*     _scroll;
      QLineEdit*      _loBox;
      QLineEdit*      _hiBox;
      QPushButton*    _autoB;
      QPushButton*    _zoomB;
      QPushButton*    _panB;
    };
  };
};

#endif
