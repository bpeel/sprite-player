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

#define N_FIREWORKS 32
/* Units per second per second */
#define GRAVITY -1.5f

#define N_SPARKS (N_FIREWORKS * 32) /* Must be a power of two */
#define TIME_PER_SPARK 0.01f /* in seconds */

#define TEXTURE_SIZE 32

typedef struct
{
  uint8_t red, green, blue, alpha;
} Color;

typedef struct
{
  float size;
  float x, y;
  float start_x, start_y;

  /* Velocities are in units per second */
  float initial_x_velocity;
  float initial_y_velocity;

  GTimer *timer;
} Firework;

typedef struct
{
  float x, y;
  float fade;
} Spark;

typedef struct _Data
{
  CoglContext *context;

  CoglGstVideoSink *sink;

  Firework fireworks[N_FIREWORKS];

  int next_spark_num;
  Spark sparks[N_SPARKS];
  GTimer *last_spark_time;

  float last_output_width;
  float last_output_height;

  CoglPipeline *base_pipeline;
  CoglPipeline *pipeline;
  CoglPrimitive *primitive;
  CoglAttributeBuffer *attribute_buffer;
} Data;

static const char
shader_source[] =
  "vec2 coord = video_pos + ((gl_PointCoord - 0.5) * point_coord_scale);\n"
  "cogl_color_out *= cogl_gst_sample_video1 (coord);\n";

static CoglTexture *
create_round_texture (CoglContext *context)
{
  uint8_t *p, *pixels;
  int x, y;
  CoglTexture2D *tex;

  p = pixels = g_malloc (TEXTURE_SIZE * TEXTURE_SIZE * 4);

  /* Generate a white circle which gets transparent towards the edges */
  for (y = 0; y < TEXTURE_SIZE; y++)
    for (x = 0; x < TEXTURE_SIZE; x++)
      {
        int dx = x - TEXTURE_SIZE / 2;
        int dy = y - TEXTURE_SIZE / 2;
        float value = sqrtf (dx * dx + dy * dy) * 255.0 / (TEXTURE_SIZE / 2);
        if (value > 255.0f)
          value = 255.0f;
        value = 255.0f - value;
        *(p++) = value;
        *(p++) = value;
        *(p++) = value;
        *(p++) = value;
      }

  tex = cogl_texture_2d_new_from_data (context,
                                       TEXTURE_SIZE, TEXTURE_SIZE,
                                       COGL_PIXEL_FORMAT_RGBA_8888_PRE,
                                       COGL_PIXEL_FORMAT_ANY,
                                       TEXTURE_SIZE * 4,
                                       pixels,
                                       NULL /* error */);

  g_free (pixels);

  return COGL_TEXTURE (tex);
}

static void
paint (CoglFramebuffer *fb,
       const CoglGstRectangle *video_output,
       void *user_data)
{
  Data *data = user_data;
  int i;
  float diff_time;
  CoglPipeline *pipeline;

  pipeline = cogl_pipeline_copy (data->pipeline);
  cogl_gst_video_sink_attach_frame (data->sink, pipeline);
  cogl_object_unref (data->pipeline);
  data->pipeline = pipeline;

  if (data->last_output_width != video_output->width ||
      data->last_output_height != video_output->height)
    {
      int location =
        cogl_pipeline_get_uniform_location (pipeline, "point_coord_scale");

      if (location != -1)
        {
          float value[2] =
            {
              TEXTURE_SIZE / video_output->width,
              TEXTURE_SIZE / video_output->height
            };

          cogl_pipeline_set_uniform_float (pipeline,
                                           location,
                                           2, /* n_components */
                                           1, /* count */
                                           value);
        }

      data->last_output_width = video_output->width;
      data->last_output_height = video_output->height;
    }

  /* Update all of the firework's positions */
  for (i = 0; i < N_FIREWORKS; i++)
    {
      Firework *firework = data->fireworks + i;

      if ((fabsf (firework->x - firework->start_x) > 2.0f) ||
          firework->y < -1.0f)
        {
          firework->size = g_random_double_range (0.001f, 0.1f);
          firework->start_x = 1.0f + firework->size;
          firework->start_y = -1.0f;
          firework->initial_x_velocity = g_random_double_range (-0.1f, -2.0f);
          firework->initial_y_velocity = g_random_double_range (0.1f, 4.0f);
          g_timer_reset (firework->timer);

          /* Fire some of the fireworks from the other side */
          if (g_random_boolean ())
            {
              firework->start_x = -firework->start_x;
              firework->initial_x_velocity = -firework->initial_x_velocity;
            }
        }

      diff_time = g_timer_elapsed (firework->timer, NULL);

      firework->x = (firework->start_x +
                     firework->initial_x_velocity * diff_time);

      firework->y = ((firework->initial_y_velocity * diff_time +
                      0.5f * GRAVITY * diff_time * diff_time) +
                     firework->start_y);
    }

  diff_time = g_timer_elapsed (data->last_spark_time, NULL);
  if (diff_time < 0.0f || diff_time >= TIME_PER_SPARK)
    {
      /* Add a new spark for each firework, overwriting the oldest ones */
      for (i = 0; i < N_FIREWORKS; i++)
        {
          Spark *spark = data->sparks + data->next_spark_num;
          Firework *firework = data->fireworks + i;

          spark->x = (firework->x +
                      g_random_double_range (-firework->size / 2.0f,
                                             firework->size / 2.0f));
          spark->y = (firework->y +
                      g_random_double_range (-firework->size / 2.0f,
                                             firework->size / 2.0f));

          data->next_spark_num = (data->next_spark_num + 1) & (N_SPARKS - 1);
        }

      /* Update the fade of each spark */
      for (i = 0; i < N_SPARKS; i++)
        {
          /* First spark is the oldest */
          Spark *spark = data->sparks + ((data->next_spark_num + i)
                                         & (N_SPARKS - 1));
          spark->fade = i / (N_SPARKS - 1.0f);
        }

      g_timer_reset (data->last_spark_time);
    }

  cogl_buffer_set_data (COGL_BUFFER (data->attribute_buffer),
                        0, /* offset */
                        data->sparks,
                        sizeof (data->sparks),
                        NULL /* error */);

  cogl_framebuffer_clear4f (fb, COGL_BUFFER_BIT_COLOR, 0, 0, 0, 1);

  cogl_framebuffer_push_rectangle_clip (fb,
                                        video_output->x,
                                        video_output->y,
                                        video_output->x +
                                        video_output->width,
                                        video_output->y +
                                        video_output->height);

  cogl_framebuffer_push_matrix (fb);

  cogl_framebuffer_translate (fb,
                              video_output->x + video_output->width / 2.0f,
                              video_output->y + video_output->height / 2.0f,
                              0.0f);
  cogl_framebuffer_scale (fb,
                          video_output->width / 2.0f,
                          video_output->height / -2.0f,
                          1.0f);

  cogl_primitive_draw (data->primitive,
                       fb,
                       data->pipeline);

  cogl_framebuffer_pop_matrix (fb);

  cogl_framebuffer_pop_clip (fb);
}

static void
create_primitive (Data *data)
{
  CoglAttribute *attributes[2];
  int i;

  data->attribute_buffer =
    cogl_attribute_buffer_new_with_size (data->context,
                                         sizeof (data->sparks));
  cogl_buffer_set_update_hint (COGL_BUFFER (data->attribute_buffer),
                               COGL_BUFFER_UPDATE_HINT_DYNAMIC);

  attributes[0] = cogl_attribute_new (data->attribute_buffer,
                                      "cogl_position_in",
                                      sizeof (Spark),
                                      G_STRUCT_OFFSET (Spark, x),
                                      2, /* n_components */
                                      COGL_ATTRIBUTE_TYPE_FLOAT);
  attributes[1] = cogl_attribute_new (data->attribute_buffer,
                                      "fade",
                                      sizeof (Spark),
                                      G_STRUCT_OFFSET (Spark, fade),
                                      1, /* n_components */
                                      COGL_ATTRIBUTE_TYPE_FLOAT);

  data->primitive =
    cogl_primitive_new_with_attributes (COGL_VERTICES_MODE_POINTS,
                                        N_SPARKS,
                                        attributes,
                                        G_N_ELEMENTS (attributes));

  for (i = 0; i < G_N_ELEMENTS (attributes); i++)
    cogl_object_unref (attributes[i]);
}

static void
create_pipeline (Data *data)
{
  CoglPipeline *pipeline;
  CoglSnippet *snippet;
  CoglTexture *texture;

  pipeline = cogl_pipeline_new (data->context);

  texture = create_round_texture (data->context);
  cogl_pipeline_set_layer_texture (pipeline, 0, texture);
  cogl_object_unref (texture);

  cogl_pipeline_set_point_size (pipeline, TEXTURE_SIZE);

  snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_VERTEX,
                              "attribute float fade;\n"
                              "varying vec2 video_pos;\n",
                              "cogl_color_out *= fade;\n"
                              "video_pos = ((cogl_position_in.xy /\n"
                              "              vec2 (2.0, -2.0)) +\n"
                              "             0.5);\n");
  cogl_pipeline_add_snippet (pipeline, snippet);
  cogl_object_unref (snippet);

  cogl_pipeline_set_layer_point_sprite_coords_enabled (pipeline,
                                                       0, /* layer */
                                                       TRUE,
                                                       NULL /* error */);

  snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_FRAGMENT,
                              "varying vec2 video_pos;\n"
                              "uniform vec2 point_coord_scale;\n"
                              "const int TEXTURE_SIZE = "
                              G_STRINGIFY (TEXTURE_SIZE) ";\n",
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
  int i;

  for (i = 0; i < N_FIREWORKS; i++)
    {
      data->fireworks[i].x = -FLT_MAX;
      data->fireworks[i].y = FLT_MAX;
      data->fireworks[i].size = 0.0f;
      data->fireworks[i].timer = g_timer_new ();
    }

  for (i = 0; i < N_SPARKS; i++)
    {
      data->sparks[i].x = 2.0f;
      data->sparks[i].y = 2.0f;
    }

  data->last_spark_time = g_timer_new ();
  data->next_spark_num = 0;

  data->context = ctx = cogl_object_ref (context);
  data->sink = g_object_ref (sink);

  create_pipeline (data);
  create_primitive (data);

  cogl_gst_video_sink_set_default_sample (data->sink, FALSE);
  cogl_gst_video_sink_set_first_layer (data->sink, 1);

  return data;
}

static void
fini (void *user_data)
{
  Data *data = user_data;

  cogl_object_unref (data->base_pipeline);
  if (data->pipeline)
    cogl_object_unref (data->pipeline);
  cogl_object_unref (data->attribute_buffer);
  cogl_object_unref (data->primitive);

  g_object_unref (data->sink);

  cogl_object_unref (data->context);

  free (data);
}

EFFECT_DEFINE ("Point sprites", sprite_effect)
