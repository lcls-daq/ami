#ifndef AmiQt_VectorArrayDesc_hh
#define AmiQt_VectorArrayDesc_hh

#include <QtGui/QWidget>
#include <QtCore/QString>

class QComboBox;
class QStringList;

namespace Ami {
  class DescEntry;

  namespace Qt {
    class AmendedRegistry;
    class ScalarPlotDesc;

    class VectorArrayDesc : public QWidget {
    public:
      VectorArrayDesc(QWidget* p, const QStringList&);
      ~VectorArrayDesc();
    public:
      QString     title() const;
      const char* expression() const;
      bool        postAnalysis() const;
      Ami::DescEntry* desc(const char*) const;
    private:
      QComboBox* _parameter;
      AmendedRegistry* _registry;
      ScalarPlotDesc* _desc;
    };
  };
};

#endif
