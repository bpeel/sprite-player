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
#include <math.h>

#include <cogl/cogl.h>
#include <cogl-gst/cogl-gst.h>

#include "effect.h"

#define N_STAR_POINTS 5

#define MIN_ADD_TIME 0.1f
#define MAX_ADD_TIME 1.0f

#define MIN_DRAW_SIZE 0.01f
#define MAX_DRAW_SIZE 0.5f

#define MIN_DROP_SPEED 0.1f
#define MAX_DROP_SPEED 1.0f

#define MIN_ROTATION_SPEED 0.0f
#define MAX_ROTATION_SPEED 360.0f

#define MIN_COORD_SCALE 0.01f
#define MAX_COORD_SCALE 0.5f /* that is the whole video */

#define MIN_WAVE_SIZE 0.0f
#define MAX_WAVE_SIZE 0.5f

typedef struct
{
  float initial_x, initial_y;
  float drop_speed;
  float rotation_speed;

  float coord_scale[2];
  float coord_offset[2];
  float tint[3];

  float draw_size;
  float wave_size;

  float start_time;
} Star;

typedef struct _Data
{
  CoglContext *context;

  CoglGstVideoSink *sink;

  CoglPrimitive *star_primitive;
  CoglPipeline *base_pipeline;
  CoglPipeline *pipeline;

  int coord_scale_location;
  int coord_offset_location;

  GList *stars;

  GTimer *timer;

  float add_time;
} Data;

static void
free_star (Data *data,
           Star *star)
{
  data->stars = g_list_remove (data->stars, star);
  g_slice_free (Star, star);
}

static void
add_star (Data *data,
          const CoglGstRectangle *video_output)
{
  Star *star = g_slice_new (Star);
  float coord_scale;
  float size_speed;
  int tint_value;
  int i;

  size_speed = g_random_double ();

  star->draw_size =
    (MAX_DRAW_SIZE - MIN_DRAW_SIZE) * size_speed + MIN_DRAW_SIZE;

  star->wave_size =
    (MAX_WAVE_SIZE - MIN_WAVE_SIZE) * (1.0f - size_speed) + MIN_WAVE_SIZE;
  if (g_random_boolean ())
    star->wave_size = -star->wave_size;

  star->initial_x = g_random_double ();
  star->initial_y = -star->draw_size;

  star->drop_speed =
    (MAX_DROP_SPEED - MIN_DROP_SPEED) * size_speed + MIN_DROP_SPEED;
  star->rotation_speed =
    g_random_double_range (MIN_ROTATION_SPEED, MAX_ROTATION_SPEED);

  coord_scale = g_random_double_range (MIN_COORD_SCALE, MAX_COORD_SCALE);

  if (video_output->width < video_output->height)
    {
      star->coord_scale[1] = coord_scale;
      star->coord_scale[0] = (coord_scale *
                              video_output->width /
                              video_output->height);
    }
  else
    {
      star->coord_scale[0] = coord_scale;
      star->coord_scale[1] = (coord_scale *
                              video_output->height /
                              video_output->width);
    }

  star->coord_offset[0] =
    g_random_double_range (star->coord_scale[0], 1.0f - star->coord_scale[0]);
  star->coord_offset[1] =
    g_random_double_range (star->coord_scale[1], 1.0f - star->coord_scale[1]);

  star->start_time = g_timer_elapsed (data->timer, NULL);

  tint_value = g_random_int_range (0, 7);

  for (i = 0; i < 3; i++)
    {
      if ((tint_value & 1))
        star->tint[i] = 1.0f;
      else
        star->tint[i] = 0.5f;

      tint_value >>= 1;
    }

  data->stars = g_list_prepend (data->stars, star);
}

static void
paint (CoglFramebuffer *fb,
       const CoglGstRectangle *video_output,
       void *user_data)
{
  Data *data = user_data;
  float elapsed = g_timer_elapsed (data->timer, NULL);
  CoglPipeline *pipeline;
  int fb_width, fb_height;
  GList *l, *next;

  pipeline = cogl_pipeline_copy (data->pipeline);
  cogl_gst_video_sink_attach_frame (data->sink, pipeline);
  cogl_object_unref (data->pipeline);
  data->pipeline = pipeline;

  if (elapsed >= data->add_time)
    {
      add_star (data, video_output);
      data->add_time =
        elapsed + g_random_double_range (MIN_ADD_TIME, MAX_ADD_TIME);
    }

  cogl_framebuffer_clear4f (fb, COGL_BUFFER_BIT_COLOR, 0, 0, 0, 1);

  cogl_framebuffer_push_matrix (fb);

  fb_width = cogl_framebuffer_get_width (fb);
  fb_height = cogl_framebuffer_get_height (fb);

  cogl_framebuffer_translate (fb, (fb_width - fb_height) / 2.0f, 0.0f, 0.0f);
  cogl_framebuffer_scale (fb, fb_height, fb_height, 1.0f);

  for (l = data->stars; l; l = next)
    {
      Star *star = l->data;
      float star_elapsed = elapsed - star->start_time;
      CoglPipeline *star_pipeline;
      float x, y, angle;

      next = l->next;

      y = star->initial_y + star_elapsed * star->drop_speed;

      /* If the star has fallen off the bottom of the screen then
       * we'll just remove it so we don't paint it again */
      if (y >= 1.0f + star->draw_size)
        {
          free_star (data, star);
          continue;
        }

      x = star->initial_x + sinf (star_elapsed / 4.0f * G_PI) * star->wave_size;

      star_pipeline = cogl_pipeline_copy (pipeline);

      cogl_framebuffer_push_matrix (fb);

      angle = star_elapsed * star->rotation_speed;

      cogl_framebuffer_translate (fb, x, y, 0.0f);
      cogl_framebuffer_scale (fb,
                              star->draw_size,
                              star->draw_size,
                              star->draw_size);
      cogl_framebuffer_rotate (fb, angle, 0.0f, 0.0f, 1.0f);

      cogl_pipeline_set_color4f (star_pipeline,
                                 star->tint[0],
                                 star->tint[1],
                                 star->tint[2],
                                 1.0f);

      cogl_pipeline_set_uniform_float (star_pipeline,
                                       data->coord_scale_location,
                                       2, /* n_components */
                                       1, /* count */
                                       star->coord_scale);
      cogl_pipeline_set_uniform_float (star_pipeline,
                                       data->coord_offset_location,
                                       2, /* n_components */
                                       1, /* count */
                                       star->coord_offset);

      cogl_primitive_draw (data->star_primitive,
                           fb,
                           star_pipeline);

      cogl_framebuffer_pop_matrix (fb);

      cogl_object_unref (star_pipeline);
    }

  cogl_framebuffer_pop_matrix (fb);
}

static void
create_pipeline (Data *data)
{
  CoglPipeline *pipeline;
  CoglSnippet *snippet;

  pipeline = cogl_pipeline_new (data->context);

  snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_TEXTURE_COORD_TRANSFORM,
                              "uniform vec2 coord_scale;\n"
                              "uniform vec2 coord_offset;\n",
                              NULL /* post */);
  cogl_snippet_set_replace (snippet,
                            "cogl_tex_coord = vec4 (cogl_position_in.st *\n"
                            "                       coord_scale +\n"
                            "                       coord_offset,\n"
                            "                       0.0,\n"
                            "                       1.0);\n");
  cogl_pipeline_add_layer_snippet (pipeline, 0, snippet);
  cogl_object_unref (snippet);

  data->coord_scale_location =
    cogl_pipeline_get_uniform_location (pipeline, "coord_scale");
  data->coord_offset_location =
    cogl_pipeline_get_uniform_location (pipeline, "coord_offset");

  data->base_pipeline = pipeline;
}

static void
create_star_primitive (Data *data)
{
  typedef CoglVertexP2 StarVertex;
  CoglAttributeBuffer *attribute_buffer;
  CoglAttribute *attributes[1];
  StarVertex vertices[N_STAR_POINTS * 2 + 2];
  int i;

  /* The first vertex is the center point so that we can use a
   * triangle fan */
  vertices[0].x = 0.0f;
  vertices[0].y = 0.0f;

  /* The remaining vertices form a circle. The radius will alternate
   * between long and short to form the points */
  for (i = 0; i <= N_STAR_POINTS * 2; i++)
    {
      StarVertex *vert = vertices + 1 + i;
      float radius = (i & 1) ? 0.38196601125010515f : 1.0f;
      float angle = 2.0f * G_PI / N_STAR_POINTS / 2.0f * i;

      vert->x = radius * sinf (angle);
      vert->y = radius * cosf (angle);
    }

  attribute_buffer =
    cogl_attribute_buffer_new (data->context,
                               sizeof (vertices),
                               vertices);
  cogl_buffer_set_update_hint (COGL_BUFFER (attribute_buffer),
                               COGL_BUFFER_UPDATE_HINT_STATIC);

  attributes[0] = cogl_attribute_new (attribute_buffer,
                                      "cogl_position_in",
                                      sizeof (StarVertex),
                                      G_STRUCT_OFFSET (StarVertex, x),
                                      2, /* n_components */
                                      COGL_ATTRIBUTE_TYPE_FLOAT);

  data->star_primitive =
    cogl_primitive_new_with_attributes (COGL_VERTICES_MODE_TRIANGLE_FAN,
                                        G_N_ELEMENTS (vertices),
                                        attributes,
                                        G_N_ELEMENTS (attributes));

  for (i = 0; i < G_N_ELEMENTS (attributes); i++)
    cogl_object_unref (attributes[i]);

  cogl_object_unref (attribute_buffer);
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

  create_pipeline (data);
  create_star_primitive (data);

  data->timer = g_timer_new ();

  return data;
}

static void
fini (void *user_data)
{
  Data *data = user_data;

  while (data->stars)
    free_star (data, data->stars->data);

  cogl_object_unref (data->base_pipeline);
  if (data->pipeline)
    cogl_object_unref (data->pipeline);
  cogl_object_unref (data->star_primitive);

  g_object_unref (data->sink);

  cogl_object_unref (data->context);

  g_timer_destroy (data->timer);

  free (data);
}

EFFECT_DEFINE ("Stars", stars_effect)
