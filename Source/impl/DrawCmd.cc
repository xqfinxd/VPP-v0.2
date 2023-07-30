#include "DrawCmd.h"

#include "Pipeline.h"

namespace VPP {

namespace impl {

DrawParam::~DrawParam() {
  if (pipeline_)
    device().destroy(pipeline_);
  if (descriptor_pool_)
    device().destroy(descriptor_pool_);
}

} // namespace impl
} // namespace VPP
