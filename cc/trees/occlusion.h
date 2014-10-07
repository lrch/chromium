// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_OCCLUSION_H_
#define CC_TREES_OCCLUSION_H_

#include "base/basictypes.h"
#include "cc/base/cc_export.h"
#include "cc/base/simple_enclosed_region.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/transform.h"

namespace cc {

class CC_EXPORT Occlusion {
 public:
  Occlusion();
  Occlusion(const gfx::Transform& draw_transform,
            const SimpleEnclosedRegion& occlusion_from_outside_target,
            const SimpleEnclosedRegion& occlusion_from_inside_target);
  Occlusion GetOcclusionWithGivenDrawTransform(
      const gfx::Transform& transform) const;

  bool HasOcclusion() const;
  bool IsOccluded(const gfx::Rect& content_rect) const;
  gfx::Rect GetUnoccludedContentRect(const gfx::Rect& content_rect) const;

 private:
  gfx::Rect GetUnoccludedRectInTargetSurface(
      const gfx::Rect& content_rect) const;

  gfx::Transform draw_transform_;
  SimpleEnclosedRegion occlusion_from_outside_target_;
  SimpleEnclosedRegion occlusion_from_inside_target_;
};

}  // namespace cc

#endif  // CC_TREES_OCCLUSION_H_
