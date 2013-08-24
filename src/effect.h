/*
 * Sprite player
 *
 * An example effect using CoglGST
 *
 * Copyright (C) 2013 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */

#ifndef _EFFECT_H
#define _EFFECT_H

#include <cogl/cogl.h>
#include <cogl-gst/cogl-gst.h>

typedef struct
{
  const char *name;

  void *
  (* init) (CoglContext *context,
            CoglGstVideoSink *sink);

  void
  (* set_up_pipeline) (CoglGstVideoSink *sink,
                       void *user_data);

  void
  (* paint) (CoglFramebuffer *fb,
             const CoglGstRectangle *video_output,
             void *user_data);

  void
  (* fini) (void *user_data);
} Effect;

#define EFFECT_DEFINE(name_str, symbol)         \
  const Effect symbol =                         \
    {                                           \
      .name = name_str,                         \
      .init = init,                             \
      .set_up_pipeline = set_up_pipeline,       \
      .paint = paint,                           \
      .fini = fini                              \
    };

#endif /* _EFFECT_H */
