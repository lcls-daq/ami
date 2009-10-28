#ifndef AmiQt_FeatureBox_hh
#define AmiQt_FeatureBox_hh

#include <QtGui/QComboBox>

#include <QtCore/QString>

namespace Ami {
  namespace Qt {
    class FeatureBox : public QComboBox {
      Q_OBJECT
    public:
      FeatureBox();
      ~FeatureBox();
    public:
      void save(char*&) const;
      void load(const char*&);
    public:
      const QString& entry() const;
    public slots:
      void change_features();
      void set_entry(const QString&);
    private:
      void _seek();
    private:
      QString _entry;
    };
  };
};

#endif
