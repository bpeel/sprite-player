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

#include "borders.h"

struct _Borders
{
  CoglPipeline *pipeline;
};

Borders *
borders_new (CoglContext *context)
{
  Borders *borders = g_slice_new (Borders);

  borders->pipeline = cogl_pipeline_new (context);
  cogl_pipeline_set_color4f (borders->pipeline, 0.0f, 0.0f, 0.0f, 1.0f);
  /* disable blending */
  cogl_pipeline_set_blend (borders->pipeline,
                           "RGBA = ADD (SRC_COLOR, 0)",
                           NULL);

  return borders;
}

void
borders_draw (Borders *borders,
              CoglFramebuffer *fb,
              const CoglGstRectangle *video_output)
{
  int fb_width = cogl_framebuffer_get_width (fb);
  int fb_height = cogl_framebuffer_get_height (fb);

  if (video_output->x)
    {
      int x = video_output->x;

      /* Letterboxed with vertical borders */
      cogl_framebuffer_draw_rectangle (fb,
                                       borders->pipeline,
                                       0, 0, x, fb_height);
      cogl_framebuffer_draw_rectangle (fb,
                                       borders->pipeline,
                                       fb_width - x,
                                       0,
                                       fb_width,
                                       fb_height);
    }
  else if (video_output->y)
    {
      int y = video_output->y;

      /* Letterboxed with horizontal borders */
      cogl_framebuffer_draw_rectangle (fb,
                                       borders->pipeline,
                                       0, 0, fb_width, y);
      cogl_framebuffer_draw_rectangle (fb,
                                       borders->pipeline,
                                       0,
                                       fb_height - y,
                                       fb_width,
                                       fb_height);
    }
}

void
borders_free (Borders *borders)
{
  cogl_object_unref (borders->pipeline);

  g_slice_free (Borders, borders);
}
