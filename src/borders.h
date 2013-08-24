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

#ifndef _BORDERS_H
#define _BORDERS_H

#include <cogl/cogl.h>
#include <cogl-gst/cogl-gst.h>

typedef struct _Borders Borders;

Borders *
borders_new (CoglContext *context);

void
borders_draw (Borders *borders,
              CoglFramebuffer *fb,
              const CoglGstRectangle *video_output);

void
borders_free (Borders *borders);

#endif /* _BORDERS_H */
