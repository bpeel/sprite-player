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

#define TEXTURE_SIZE 4096

#define N_SQUARES 100

typedef struct _Data
{
  CoglContext *context;

  CoglGstVideoSink *sink;

  CoglPipeline *base_pipeline;
  CoglPipeline *pipeline;

  Borders *borders;
} Data;

static const char
shader_source[] =
  "const float PI = " G_STRINGIFY (G_PI) ";\n"
  "vec2 coords = cogl_tex_coord0_in.st;\n"
  "coords += sin (coords * PI * 2.0 * 8.0) / 30.0;\n"
  "cogl_color_out *= cogl_gst_sample_video0 (coords);\n";

static void
paint (CoglFramebuffer *fb,
       const CoglGstRectangle *video_output,
       void *user_data)
{
  Data *data = user_data;
  CoglPipeline *pipeline;

  pipeline = cogl_pipeline_copy (data->pipeline);
  cogl_gst_video_sink_attach_frame (data->sink, pipeline);
  cogl_object_unref (data->pipeline);
  data->pipeline = pipeline;

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
create_pipeline (Data *data)
{
  CoglPipeline *pipeline;
  CoglSnippet *snippet;

  pipeline = cogl_pipeline_new (data->context);

  /* Disable blending */
  cogl_pipeline_set_blend (pipeline,
                           "RGBA = ADD (SRC_COLOR, 0)", NULL);

  snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_FRAGMENT,
                              NULL, /* declarations */
                              shader_source);
  cogl_pipeline_add_snippet (pipeline, snippet);
  cogl_object_unref (snippet);

  data->base_pipeline = pipeline;
}

static void
set_up_pipeline (CoglGstVideoSink *sink,
                 void *user_data)
{
  Data *data = (Data *) user_data;

  if (data->pipeline)
    cogl_object_unref (data->pipeline);

  data->pipeline = cogl_pipeline_copy (data->base_pipeline);
  cogl_gst_video_sink_setup_pipeline (data->sink, data->pipeline);
}

static void *
init (CoglContext *context,
      CoglGstVideoSink *sink)
{
  Data *data = g_new0 (Data, 1);
  CoglContext *ctx;

  data->context = ctx = cogl_object_ref (context);
  data->sink = g_object_ref (sink);

  data->borders = borders_new (context);

  create_pipeline (data);

  cogl_gst_video_sink_set_default_sample (data->sink, FALSE);

  return data;
}

static void
fini (void *user_data)
{
  Data *data = user_data;

  borders_free (data->borders);

  cogl_object_unref (data->base_pipeline);
  if (data->pipeline)
    cogl_object_unref (data->pipeline);

  g_object_unref (data->sink);

  cogl_object_unref (data->context);

  free (data);
}

EFFECT_DEFINE ("Wavey", wavey_effect)
