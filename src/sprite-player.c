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

#include "effects.h"

typedef struct _Data
{
  CoglContext *context;
  CoglFramebuffer *fb;
  CoglGstVideoSink *sink;
  int onscreen_width;
  int onscreen_height;
  CoglGstRectangle video_output;
  bool draw_ready;
  bool frame_ready;
  GMainLoop *main_loop;

  const Effect *current_effect;
  void *effect_data;
} Data;

static gboolean
bus_watch (GstBus *bus,
           GstMessage *msg,
           void *user_data)
{
  Data *data = (Data *) user_data;

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
paint (Data *data)
{
  data->current_effect->paint (data->fb,
                               &data->video_output,
                               data->effect_data);

  cogl_onscreen_swap_buffers (COGL_ONSCREEN (data->fb));
}

static void
check_draw (Data *data)
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
frame_callback (CoglOnscreen *onscreen,
                CoglFrameEvent event,
                CoglFrameInfo *info,
                void *user_data)
{
  Data *data = user_data;

  if (event == COGL_FRAME_EVENT_SYNC)
    {
      data->draw_ready = TRUE;
      check_draw (data);
    }
}

static void
new_frame_cb (CoglGstVideoSink *sink,
              Data *data)
{
  data->frame_ready = TRUE;
  check_draw (data);
}

static void
update_video_output (Data *data)
{
  CoglGstRectangle available;

  available.x = 0;
  available.y = 0;
  available.width = data->onscreen_width;
  available.height = data->onscreen_height;

  cogl_gst_video_sink_fit_size (data->sink,
                                &available,
                                &data->video_output);
}

static void
resize_callback (CoglOnscreen *onscreen,
                 int width,
                 int height,
                 void *user_data)
{
  Data *data = user_data;

  data->onscreen_width = width;
  data->onscreen_height = height;

  cogl_framebuffer_orthographic (data->fb, 0, 0, width, height, -1, 100);

  if (cogl_gst_video_sink_is_ready (data->sink))
    update_video_output (data);
}

static void
set_up_pipeline (gpointer instance,
                 gpointer user_data)
{
  Data *data = (Data *) user_data;

  update_video_output (data);

  data->current_effect->set_up_pipeline (data->sink,
                                         data->effect_data);
}

static void
clear_effect (Data *data)
{
  if (data->current_effect)
    {
      data->current_effect->fini (data->effect_data);
      data->current_effect = NULL;
    }
}

static void
set_effect (Data *data,
            const Effect *effect)
{
  clear_effect (data);

  /* Reset the sink options to a known state */
  cogl_gst_video_sink_set_default_sample (data->sink, TRUE);
  cogl_gst_video_sink_set_first_layer (data->sink, 0);

  data->effect_data = effect->init (data->context, data->sink);
  data->current_effect = effect;

  /* If the pipeline is already ready then we can immediately set it
   * up */
  if (cogl_gst_video_sink_is_ready (data->sink))
    effect->set_up_pipeline (data->sink, data->effect_data);
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

  memset (&data, 0, sizeof (Data));

  /* Set the necessary cogl elements */

  data.context = ctx = cogl_context_new (NULL, NULL);

  onscreen = cogl_onscreen_new (ctx, 800, 600);
  cogl_onscreen_set_resizable (onscreen, TRUE);
  cogl_onscreen_add_resize_callback (onscreen, resize_callback, &data, NULL);
  cogl_onscreen_show (onscreen);

  data.fb = COGL_FRAMEBUFFER (onscreen);

  /* Intialize GStreamer */

  gst_init (&argc, &argv);

  data.sink = cogl_gst_video_sink_new (ctx);

  pipeline = gst_pipeline_new ("gst-player");
  bin = gst_element_factory_make ("playbin", "bin");

  if (argc < 2)
    uri = "http://docs.gstreamer.com/media/sintel_trailer-480p.webm";
  else
    uri = argv[1];

  g_object_set (G_OBJECT (bin), "video-sink", GST_ELEMENT (data.sink), NULL);

  gst_bin_add (GST_BIN (pipeline), bin);

  g_object_set (G_OBJECT (bin), "uri", uri, NULL);

  set_effect (&data, effects[0]);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, bus_watch, &data);

  data.main_loop = g_main_loop_new (NULL, FALSE);

  cogl_source = cogl_glib_source_new (ctx, G_PRIORITY_DEFAULT);
  g_source_attach (cogl_source, NULL);

  g_signal_connect (data.sink, "pipeline-ready",
                    G_CALLBACK (set_up_pipeline), &data);

  g_signal_connect (data.sink, "new-frame", G_CALLBACK (new_frame_cb), &data);

  cogl_onscreen_add_frame_callback (COGL_ONSCREEN (data.fb),
                                    frame_callback,
                                    &data,
                                    NULL);

  data.draw_ready = TRUE;
  data.frame_ready = FALSE;

  g_main_loop_run (data.main_loop);

  clear_effect (&data);

  g_source_destroy (cogl_source);
  g_source_unref (cogl_source);

  g_main_loop_unref (data.main_loop);

  return 0;
}
