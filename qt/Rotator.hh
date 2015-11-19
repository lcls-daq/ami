#ifndef AmiQt_Rotator_hh
#define AmiQt_Rotator_hh

#include "ami/data/ConfigureRequestor.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/DescImage.hh"

class QComboBox;
class QWidget;

namespace Ami {
  class RotateImage;
  namespace Qt {
    class Client;
    class Rotator {
    public:
      Rotator(Client&);
      ~Rotator();
    public:
      void save(char*&) const;
      void load(const char*&);
    public:
      Ami::Rotation rotation () const;
      QWidget*      widget   () const;
      QComboBox*    box      () const;
      void          prototype(const DescEntry&);
      unsigned      configure(char*&,
                              unsigned,
                              unsigned&,
                              ConfigureRequest::Source&);
    private:
      QComboBox*         _roBox;
      QWidget*           _widget;
      DescImage          _prototype;
      RotateImage*       _op;
      ConfigureRequestor _req;
    };
  };
};

#endif
