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

#include "config.h"

#include <stdbool.h>

#include <cogl/cogl.h>
#include <cogl-gst/cogl-gst.h>

#include "effect.h"
#include "borders.h"

typedef struct _Data
{
  CoglContext *context;

  CoglGstVideoSink *sink;

  Borders *borders;
} Data;

static void
paint (CoglFramebuffer *fb,
       const CoglGstRectangle *video_output,
       void *user_data)
{
  Data *data = user_data;
  CoglPipeline *pipeline =
    cogl_gst_video_sink_get_pipeline (data->sink);

  borders_draw (data->borders, fb, video_output);

  cogl_framebuffer_draw_rectangle (fb,
                                   pipeline,
                                   video_output->x,
                                   video_output->y,
                                   video_output->x +
                                   video_output->width,
                                   video_output->y +
                                   video_output->height);
}

static void
set_up_pipeline (CoglGstVideoSink *sink,
                 void *user_data)
{
}

static void *
init (CoglContext *context,
      CoglGstVideoSink *sink)
{
  Data *data = g_new0 (Data, 1);

  data->context = cogl_object_ref (context);
  data->sink = g_object_ref (sink);
  data->borders = borders_new (context);

  return data;
}

static void
fini (void *user_data)
{
  Data *data = user_data;

  borders_free (data->borders);

  g_object_unref (data->sink);

  cogl_object_unref (data->context);

  free (data);
}

EFFECT_DEFINE ("No effect", no_effect)
