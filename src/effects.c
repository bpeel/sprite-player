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

#include "effects.h"

extern Effect no_effect;
extern Effect sprite_effect;
extern Effect squares_effect;
extern Effect wavey_effect;
extern Effect edge_effect;
extern Effect stars_effect;

const Effect * const
effects[N_EFFECTS] =
  {
    &no_effect,
    &sprite_effect,
    &squares_effect,
    &wavey_effect,
    &edge_effect,
    &stars_effect,
  };

_Static_assert (G_N_ELEMENTS (effects) == N_EFFECTS,
                "If you add an effect, don't forget to update N_EFFECTS "
                "in effects.h");
