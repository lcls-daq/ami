#include "ami/qt/ImageOffload.hh"

#include "ami/qt/ImageColorControl.hh"
#include "ami/qt/QtImage.hh"

#include "ami/service/Task.hh"
#include "ami/service/TaskObject.hh"
#include "ami/service/Routine.hh"
#include "ami/service/Semaphore.hh"

namespace Ami {
  namespace Qt {
    class RenderRoutine : public Ami::Routine {
    public:
      RenderRoutine(ImageOffload* frame) : _frame(frame) {}
    public:
      void routine() { _frame->_start_render(); delete this; }
    private:
      ImageOffload* _frame;
    };

    class RenderComplete : public Ami::Routine {
    public:
      RenderComplete(ImageOffload* frame, QImage* qimage) :
        _frame(frame), _qimage(qimage) {}
    public:
      void routine() { _frame->_render_complete(_qimage); delete this; }
    private:
      ImageOffload* _frame;
      QImage*     _qimage;
    };

    class SyncRoutine : public Ami::Routine {
    public:
      SyncRoutine(Ami::Semaphore& sem) : _sem(sem) {}
    public:
      void routine() { _sem.give(); }
    private:
      Ami::Semaphore& _sem;
    };
  }
}

using namespace Ami::Qt;

ImageOffload::ImageOffload(const ImageColorControl& control) :
  _image  (0),
  _control(control),
  _task   (new Ami::Task(Ami::TaskObject("imgoff"))),
  _pending(false)
{
  connect(this, SIGNAL(_image_rendered(QImage*)), this, SLOT(_render_canvas(QImage*)));
}

ImageOffload::~ImageOffload() { _task->destroy(); }

void ImageOffload::qimage(QtImage* q)
{
  _pending=false;
  _render_sync();
  _image = q;
  if (!q)
    _render_sync();
}

void ImageOffload::_render_sync()
{
  Ami::Semaphore sem(Ami::Semaphore::EMPTY);
  _task->call( new SyncRoutine(sem) );
  sem.take();
}

void ImageOffload::_render()
{
  if (_qimage)
    _task->call( new RenderRoutine(this) );
}

void ImageOffload::_start_render()
{
  if (_qimage) {
    QImage* output = _qimage->image(_control.pedestal(),_control.scale(),_control.linear());
          
    if (output) {
      _pending=false;
      _render_image(*output);
      emit _image_rendered(output);
    }
    else
      _pending=true;
  }
}

void ImageOffload::_render_canvas(QImage* output)
{
  if (_qimage && _qimage->owns(output)) {
    _render_pixmap(*output);
    _task->call(new RenderComplete(this, output));
  }
}

void ImageOffload::_render_complete(QImage* output)
{
  if (_qimage) {
    _qimage->release(output);
    if (_pending)
      _start_render();
  }
}
