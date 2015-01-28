#ifndef AmiQt_ImageOffload_hh
#define AmiQt_ImageOffload_hh

class QImage;

namespace Ami {
  namespace Qt {
    class ImageOffload {
    public:
      virtual ~ImageOffload() {}
    public:
      virtual void render_image (QImage&) = 0;
      virtual void render_pixmap(QImage&) = 0;
    };
  };
};

#endif
