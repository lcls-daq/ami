#ifndef AmiQt_QOnline_hh
#define AmiQt_QOnline_hh

#include "ami/service/Poll.hh"

#include <QtGui/QGroupBox>
#include <QtCore/QStringList>

#include <vector>

class QCheckBox;
class QLabel;
class QPushButton;
class QButtonGroup;
class QGridLayout;

namespace Ami {
  namespace Qt {
    class ShmServer;
    class QOnline : public QGroupBox,
		    public Poll
    {
      Q_OBJECT
    public:
      QOnline(const char* nodes, unsigned platform);
      ~QOnline();
    public:
      int processTmo();
      void update();
      void remove(ShmServer*);
    public slots:
      void _update();
      void _remove(int);
      void update_mask(int);
      void apply_mask ();
    signals:
      void updated();
      void removed(int);
      void exit();
    private:
      std::vector<ShmServer*> _servers;
      struct RowWidgets {
	std::vector<QCheckBox*> _box;
	QLabel* _events;
	QLabel* _dmg;
      };
      bool _lmajor;
      QPushButton* _applyB;
      QGridLayout* _layout;
      QButtonGroup* _mask_group;
      std::vector<RowWidgets> _rows;
      QStringList _qnodes;
      unsigned    _platform;
    };
  };
};

#endif
      
