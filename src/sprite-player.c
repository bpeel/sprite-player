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

#include <stdbool.h>

#include <cogl/cogl.h>
#include <cogl-gst/cogl-gst.h>

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
  CoglFramebuffer *fb;
  CoglGstVideoSink *sink;
  CoglGstRectangle video_output;
  bool draw_ready;
  bool frame_ready;
  GMainLoop *main_loop;

  Firework fireworks[N_FIREWORKS];

  int next_spark_num;
  Spark sparks[N_SPARKS];
  GTimer *last_spark_time;

  CoglPipeline *base_pipeline;
  CoglPipeline *pipeline;
  CoglPrimitive *primitive;
  CoglAttributeBuffer *attribute_buffer;
} Data;

static const char
shader_source[] =
  "vec2 coord = gl_FragCoord.st / vec2 (800.0, 600.0);\n"
  "coord.t = 1.0 - coord.t;\n"
  "cogl_color_out *=\n"
  "  cogl_gst_sample_video1 (coord);\n";

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
paint (Data *data)
{
  int i;
  float diff_time;
  CoglPipeline *pipeline;

  pipeline = cogl_pipeline_copy (data->pipeline);
  cogl_gst_video_sink_attach_frame (data->sink, pipeline);
  cogl_object_unref (data->pipeline);
  data->pipeline = pipeline;

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

  cogl_framebuffer_clear4f (data->fb, COGL_BUFFER_BIT_COLOR, 0, 0, 0, 1);

  cogl_primitive_draw (data->primitive,
                       data->fb,
                       data->pipeline);

  cogl_onscreen_swap_buffers (COGL_ONSCREEN (data->fb));
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
                              "attribute float fade;\n",
                              "cogl_color_out *= fade;\n");
  cogl_pipeline_add_snippet (pipeline, snippet);
  cogl_object_unref (snippet);

  cogl_pipeline_set_layer_point_sprite_coords_enabled (pipeline,
                                                       0, /* layer */
                                                       TRUE,
                                                       NULL /* error */);

  snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_FRAGMENT,
                              NULL, /* declarations */
                              shader_source);
  cogl_pipeline_add_snippet (pipeline, snippet);
  cogl_object_unref (snippet);

  data->base_pipeline = pipeline;
}

static gboolean
_bus_watch (GstBus *bus,
            GstMessage *msg,
            void *user_data)
{
  Data *data = (Data*) user_data;
  switch (GST_MESSAGE_TYPE (msg))
    {
      case GST_MESSAGE_EOS:
        {
          g_main_loop_quit (data->main_loop);
          break;
        }
      case GST_MESSAGE_ERROR:
        {
          char *debug;
          GError *error = NULL;

          gst_message_parse_error (msg, &error, &debug);
          g_free (debug);

          if (error != NULL)
            {
              g_error ("Playback error: %s\n", error->message);
              g_error_free (error);
            }
          g_main_loop_quit (data->main_loop);
          break;
        }
      default:
        break;
    }

  return TRUE;
}

static void
_check_draw (Data *data)
{
  /* The frame is only drawn once we know that a new buffer is ready
   * from GStreamer and that Cogl is ready to accept some new
   * rendering */
  if (data->draw_ready && data->frame_ready)
    {
      paint (data);
      data->draw_ready = FALSE;
      data->frame_ready = FALSE;
    }
}

static void
_frame_callback (CoglOnscreen *onscreen,
                 CoglFrameEvent event,
                 CoglFrameInfo *info,
                 void *user_data)
{
  Data *data = user_data;

  if (event == COGL_FRAME_EVENT_SYNC)
    {
      data->draw_ready = TRUE;
      _check_draw (data);
    }
}

static void
_new_frame_cb (CoglGstVideoSink *sink,
               Data *data)
{
  data->frame_ready = TRUE;
  _check_draw (data);
}

/*
  A callback like this should be attached to the cogl-pipeline-ready
  signal. This way requesting the cogl pipeline before its creation
  by the sink is avoided. At this point, user textures and snippets can
  be added to the cogl pipeline.
*/

static void
_set_up_pipeline (gpointer instance,
                  gpointer user_data)
{
  Data* data = (Data*) user_data;

  /*
    The cogl-gst sink, depending on the video format, can use up to 3 texture
    layers to render a frame. To avoid overwriting frame data, the first
    free layer in the cogl pipeline needs to be queried before adding any
    additional textures.
  */

  if (data->pipeline)
    cogl_object_unref (data->pipeline);

  data->pipeline = cogl_pipeline_copy (data->base_pipeline);
  cogl_gst_video_sink_setup_pipeline (data->sink, data->pipeline);

  cogl_onscreen_add_frame_callback (COGL_ONSCREEN (data->fb), _frame_callback,
                                    data, NULL);

  /*
     The cogl-gst-new-frame signal is emitted when the cogl-gst sink has
     retrieved a new frame and attached it to the cogl pipeline. This can be
     used to make sure cogl doesn't do any unnecessary drawing i.e. keeps to the
     frame-rate of the video.
  */

  g_signal_connect (data->sink, "new-frame", G_CALLBACK (_new_frame_cb), data);
}

int
main (int argc,
      char **argv)
{
  Data data;
  CoglContext *ctx;
  CoglOnscreen *onscreen;
  GstElement *pipeline;
  GstElement *bin;
  GSource *cogl_source;
  GstBus *bus;
  char *uri;
  int i;

  memset (&data, 0, sizeof (Data));

  for (i = 0; i < N_FIREWORKS; i++)
    {
      data.fireworks[i].x = -FLT_MAX;
      data.fireworks[i].y = FLT_MAX;
      data.fireworks[i].size = 0.0f;
      data.fireworks[i].timer = g_timer_new ();
    }

  for (i = 0; i < N_SPARKS; i++)
    {
      data.sparks[i].x = 2.0f;
      data.sparks[i].y = 2.0f;
    }

  data.last_spark_time = g_timer_new ();
  data.next_spark_num = 0;

  /* Set the necessary cogl elements */

  data.context = ctx = cogl_context_new (NULL, NULL);

  onscreen = cogl_onscreen_new (ctx, 800, 600);
  cogl_onscreen_show (onscreen);

  create_pipeline (&data);
  create_primitive (&data);

  data.fb = COGL_FRAMEBUFFER (onscreen);

  /* Intialize GStreamer */

  gst_init (&argc, &argv);

  /*
     Create the cogl-gst video sink by calling the cogl_gst_video_sink_new
     function and passing it a CoglContext (this is used to create the
     CoglPipeline and the texures for each frame). Alternatively you can use
     gst_element_factory_make ("coglsink", "some_name") and then set the
     context with cogl_gst_video_sink_set_context.
  */

  data.sink = cogl_gst_video_sink_new (ctx);

  cogl_gst_video_sink_set_default_sample (data.sink, FALSE);
  cogl_gst_video_sink_set_first_layer (data.sink, 1);

  pipeline = gst_pipeline_new ("gst-player");
  bin = gst_element_factory_make ("playbin", "bin");

  if (argc < 2)
    uri = "http://docs.gstreamer.com/media/sintel_trailer-480p.webm";
  else
    uri = argv[1];

  g_object_set (G_OBJECT (bin), "video-sink", GST_ELEMENT (data.sink), NULL);


  gst_bin_add (GST_BIN (pipeline), bin);

  g_object_set (G_OBJECT (bin), "uri", uri, NULL);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, _bus_watch, &data);

  data.main_loop = g_main_loop_new (NULL, FALSE);

  cogl_source = cogl_glib_source_new (ctx, G_PRIORITY_DEFAULT);
  g_source_attach (cogl_source, NULL);

  /*
    The cogl-pipeline-ready signal tells you when the cogl pipeline is
    initialized i.e. when cogl-gst has figured out the video format and
    is prepared to retrieve and attach the first frame of the video.
  */

  g_signal_connect (data.sink, "pipeline-ready",
                    G_CALLBACK (_set_up_pipeline), &data);

  data.draw_ready = TRUE;
  data.frame_ready = FALSE;

  g_main_loop_run (data.main_loop);

  cogl_object_unref (data.base_pipeline);
  if (data.pipeline)
    cogl_object_unref (data.pipeline);
  cogl_object_unref (data.attribute_buffer);
  cogl_object_unref (data.primitive);

  g_source_destroy (cogl_source);
  g_source_unref (cogl_source);

  g_main_loop_unref (data.main_loop);

  return 0;
}
