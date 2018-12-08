CC=riscv32-unknown-elf-gcc
CFLAGS=-O2 -I.
DEPS = \
	am_map.h		\
	d_englsh.h	\
	d_event.h		\
	d_french.h	\
	d_items.h		\
	d_main.h		\
	d_net.h			\
	d_player.h	\
	d_textur.h	\
	d_think.h		\
	d_ticcmd.h	\
	doomdata.h	\
	doomdef.h		\
	doomstat.h	\
	doomtype.h	\
	dstrings.h	\
	f_finale.h	\
	f_wipe.h		\
	g_game.h		\
	hu_lib.h		\
	hu_stuff.h	\
	i_sound.h		\
	i_system.h	\
	i_net.h			\
	i_video.h		\
	info.h			\
	m_argv.h		\
	m_bbox.h		\
	m_cheat.h		\
	m_fixed.h		\
	m_menu.h		\
	m_misc.h		\
	m_random.h	\
	m_swap.h		\
	p_inter.h		\
	p_local.h		\
	p_mobj.h		\
	p_pspr.h		\
	p_saveg.h		\
	p_setup.h		\
	p_spec.h		\
	p_tick.h		\
	r_bsp.h			\
	r_data.h		\
	r_defs.h		\
	r_draw.h		\
	r_local.h		\
	r_main.h		\
	r_plane.h		\
	r_segs.h		\
	r_sky.h			\
	r_state.h		\
	r_things.h	\
	s_sound.h		\
	sounds.h		\
	st_lib.h		\
	st_stuff.h	\
	tables.h		\
	v_video.h		\
	w_wad.h			\
	wi_stuff.h	\
	z_zone.h

OBJS = \
	rv_av_api.o	\
	am_map.o		\
	d_items.o		\
	d_main.o		\
	d_net.o			\
	doomdef.o		\
	doomstat.o	\
	dstrings.o	\
	f_finale.o	\
	f_wipe.o		\
	g_game.o		\
	hu_lib.o		\
	hu_stuff.o	\
	i_main.o		\
	i_sound.o		\
	i_system.o	\
	i_net.o			\
	i_video.o		\
	info.o			\
	m_argv.o		\
	m_bbox.o		\
	m_cheat.o		\
	m_fixed.o		\
	m_menu.o		\
	m_misc.o		\
	m_random.o	\
	m_swap.o		\
	p_ceilng.o	\
	p_doors.o		\
	p_enemy.o		\
	p_floor.o		\
	p_inter.o		\
	p_lights.o	\
	p_map.o			\
	p_maputl.o	\
	p_mobj.o		\
	p_plats.o		\
	p_pspr.o		\
	p_saveg.o		\
	p_setup.o		\
	p_sight.o		\
	p_spec.o		\
	p_switch.o	\
	p_telept.o	\
	p_tick.o		\
	p_user.o		\
	r_bsp.o			\
	r_data.o		\
	r_draw.o		\
	r_main.o		\
	r_plane.o		\
	r_segs.o		\
	r_sky.o			\
	r_things.o	\
	s_sound.o		\
	sounds.o		\
	st_lib.o		\
	st_stuff.o	\
	tables.o		\
	v_video.o		\
	w_wad.o			\
	wi_stuff.o	\
	z_zone.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

doom: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o doom
