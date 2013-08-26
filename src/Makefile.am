bin_PROGRAMS = sprite-player

AM_CFLAGS = \
	$(COGL_CFLAGS) \
	$(COGL_GST_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(WARNING_FLAGS) \
	$(NULL)

effects = \
	edge-effect.c \
	no-effect.c \
	sprite-effect.c \
	stars-effect.c \
	squares-effect.c \
	wavey-effect.c \
	$(NULL)

sprite_player_SOURCES = \
	borders.c \
	borders.h \
	effect.h \
	effects.c \
	effects.h \
	sprite-player.c \
	$(effects) \
	$(NULL)

sprite_player_LDADD = \
	$(COGL_LIBS) \
	$(COGL_GST_LIBS) \
	$(GLIB_LIBS) \
	$(NULL)
