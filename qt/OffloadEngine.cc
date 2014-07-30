#include "ami/qt/OffloadEngine.hh"

#include "ami/qt/ImageOffload.hh"
#include "ami/qt/ImageColorControl.hh"
#include "ami/qt/QtImage.hh"

#include "ami/service/Task.hh"
#include "ami/service/TaskObject.hh"
#include "ami/service/Routine.hh"
#include "ami/service/Semaphore.hh"

static bool _enabled=true;

namespace Ami {
  namespace Qt {
    class RenderRoutine : public Ami::Routine {
    public:
      RenderRoutine(OffloadEngine* engine) : _engine(engine) {}
    public:
      void routine() { _engine->_start_render(); delete this; }
    private:
      OffloadEngine* _engine;
    };

    class RenderComplete : public Ami::Routine {
    public:
      RenderComplete(OffloadEngine* engine, QImage* qimage) :
        _engine(engine), _qimage(qimage) {}
    public:
      void routine() { _engine->_render_complete(_qimage); delete this; }
    private:
      OffloadEngine* _engine;
      QImage*        _qimage;
    };

    class SyncRoutine : public Ami::Routine {
    public:
      SyncRoutine(Ami::Semaphore& sem) : _sem(sem) {}
    public:
      void routine() { _sem.give(); delete this; }
    private:
      Ami::Semaphore& _sem;
    };
  }
}

using namespace Ami::Qt;

OffloadEngine::OffloadEngine(ImageOffload& offload,
                             const ImageColorControl& control) :
  _offload(offload),
  _image  (0),
  _control(control),
  _task   (new Ami::Task(Ami::TaskObject("imgoff"))),
  _pending(false)
{
  connect(this, SIGNAL(_image_rendered(QImage*)), this, SLOT(_render_canvas(QImage*)));
}

OffloadEngine::~OffloadEngine() { _task->destroy_b(); }

void OffloadEngine::disable() { _enabled=false; }

void OffloadEngine::qimage(QtImage* q)
{
  _pending=false;
  render_sync();
  _image = q;
  if (!q)
    render_sync();
}

void OffloadEngine::render_sync()
{
  if (_enabled) {
    Ami::Semaphore sem(Ami::Semaphore::EMPTY);
    _task->call( new SyncRoutine(sem) );
    sem.take();
  }
}

void OffloadEngine::render()
{
  if (_image) {
    if (_enabled)
      _task->call( new RenderRoutine(this) );
    else {
      QImage* output = _image->image(_control.pedestal(),_control.scale(),_control.linear());
      if (output) {
        _offload.render_image(*output);
        _offload.render_pixmap(*output);
        _image->release(output);
      }
    }
  }
}

void OffloadEngine::_start_render()
{
  if (_image) {
    QImage* output = _image->image(_control.pedestal(),_control.scale(),_control.linear());
          
    if (output) {
      _pending=false;
      _offload.render_image(*output);
      emit _image_rendered(output);
    }
    else
      _pending=true;
  }
}

void OffloadEngine::_render_canvas(QImage* output)
{
  if (_image && _image->owns(output)) {
    _offload.render_pixmap(*output);
    _task->call(new RenderComplete(this, output));
  }
}

void OffloadEngine::_render_complete(QImage* output)
{
  if (_image) {
    _image->release(output);
    if (_pending)
      _start_render();
  }
}

