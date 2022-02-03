#include "ami/event/JungfrauAlignment.hh"

using namespace Ami::Alignment;

static const double pixel_size = 75.0; // um

Jungfrau::Jungfrau(const Pds::DetInfo& det, unsigned nelems,
                   unsigned rows, unsigned columns, unsigned index) :
  Detector(det, "JFCAMERA", "JUNGFRAU",
           nelems, pixel_size,
           columns, rows,
           index)
{
  if (_use_default) load_default();
}

Jungfrau::~Jungfrau()
{}
