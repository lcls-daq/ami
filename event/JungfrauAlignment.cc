#include "ami/event/JungfrauAlignment.hh"

using namespace Ami::Alignment;

static const double pixel_size = 75.0; // um

Jungfrau::Jungfrau(const Pds::DetInfo& det, unsigned nelems,
                   unsigned rows, unsigned columns, unsigned index) :
  Detector(det, "JFCAMERA:V1", "JUNGFRAU:V1",
           nelems, pixel_size,
           rows, columns,
           index)
{}

Jungfrau::~Jungfrau()
{}
