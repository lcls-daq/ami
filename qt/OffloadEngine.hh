#ifndef AmiQt_OffloadEngine_hh
#define AmiQt_OffloadEngine_hh

#include <QtCore/QObject>

class QImage;

namespace Ami {
  class Task;

  namespace Qt {
    class ImageColorControl;
    class ImageOffload;
    class QtImage;

    class OffloadEngine : public QObject {
      Q_OBJECT
    public:
      OffloadEngine(ImageOffload&, ImageColorControl&);
      virtual ~OffloadEngine();
    public:
      ImageColorControl& control() const { return _control; }
      QtImage* qimage() const { return _image; }
      void     qimage(QtImage*);
      void     render_sync();
      void     render();
    public:
      static void disable();
    private:
      void     _start_render();
      void     _render_complete(QImage*);
    private slots:
      void     _render_canvas  (QImage*);
    signals:
      void     _image_rendered (QImage*);
    private:
      ImageOffload&            _offload;
      QtImage*                 _image;
      ImageColorControl&       _control;
      Task*                    _task;
      bool                     _pending;
      friend class RenderRoutine;
      friend class RenderComplete;
    };
  };
};

#endif
