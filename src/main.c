/*! \file main.c
 *  \brief Main file of SuperDerpy engine.
 *
 *   Contains basic functions shared by all views.
 */
/*
 * Copyright (c) Sebastian Krzyszkowiak <dos@dosowisko.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include <stdio.h>
#include <math.h>
#include <getopt.h>
#include <locale.h>
#include "menu.h"
#include "loading.h"
#include "about.h"
#include "intro.h"
#include "map.h"
#include "level.h"
#include "pause.h"
#include "config.h"

/*! \brief Macro for preloading gamestate.
 *
 *  Preloading of state happens when loading screen is displayed.
 */
#define PRELOAD_STATE(state, name) case state:\
	PrintConsole(game, "Preload %s...", #state); DrawConsole(game); al_flip_display(); name ## _Preload(game); break;
/*! \brief Macro for unloading gamestate.
 *
 *  Unloading of state happens after it's fadeout.
 */
#define UNLOAD_STATE(state, name) case state:\
	PrintConsole(game, "Unload %s...", #state); name ## _Unload(game); break;
/*! \brief Macro for loading gamestate.
 *
 *  Loading of state means setting it as active and running it.
 */
#define LOAD_STATE(state, name) case state:\
	PrintConsole(game, "Load %s...", #state); name ## _Load(game); break;
/*! \brief Macro for sending keydown events to gamestate. */
#define KEYDOWN_STATE(state, name) else if (game.gamestate==state) { if (name ## _Keydown(&game, &ev)) break; }
/*! \brief Macro for drawing active gamestate. */
#define DRAW_STATE(state, name) case state:\
	name ## _Draw(game); break;

double old_time = 0, fps;
int frames_done = 0;

void al_draw_text_with_shadow(ALLEGRO_FONT *font, ALLEGRO_COLOR color, float x, float y, int flags, char const *text) {
	al_draw_text(font, al_map_rgba(0,0,0,128), x+1, y+1, flags, text);
	al_draw_text(font, color, x, y, flags, text);
}

void PrintConsole(struct Game *game, char* format, ...) {
	va_list vl;
	va_start(vl, format);
	char text[255] = {};
	vsprintf(text, format, vl);
	va_end(vl);
	if (game->debug) printf("%s\n", text);
	ALLEGRO_BITMAP *con = al_create_bitmap(al_get_bitmap_width(game->console), al_get_bitmap_height(game->console));
	al_set_target_bitmap(con);
	al_clear_to_color(al_map_rgba(0,0,0,80));
	al_draw_bitmap_region(game->console, 0, al_get_bitmap_height(game->console)*0.2, al_get_bitmap_width(game->console), al_get_bitmap_height(game->console)*0.8, 0, 0, 0);
	al_draw_text(game->font_console, al_map_rgb(255,255,255), al_get_display_width(game->display)*0.005, al_get_bitmap_height(game->console)*0.81, ALLEGRO_ALIGN_LEFT, text);
	al_set_target_bitmap(game->console);
	al_clear_to_color(al_map_rgba(0,0,0,0));
	al_draw_bitmap(con, 0, 0, 0);
	al_set_target_bitmap(al_get_backbuffer(game->display));
	al_destroy_bitmap(con);
}

void DrawConsole(struct Game *game) {
	if (game->showconsole) {
		al_draw_bitmap(game->console, 0, 0, 0);
		double game_time = al_get_time();
		if(game_time - old_time >= 1.0) {
			fps = frames_done / (game_time - old_time);
			frames_done = 0;
			old_time = game_time;
		}
		char sfps[6] = { };
		sprintf(sfps, "%.0f", fps);
		al_draw_text_with_shadow(game->font, al_map_rgb(255,255,255), al_get_display_width(game->display)*0.99, 0, ALLEGRO_ALIGN_RIGHT, sfps);
	}
	frames_done++;
}

void PreloadGameState(struct Game *game) {
	if ((game->loadstate==GAMESTATE_MENU) && (game->menu.loaded)) {
		PrintConsole(game, "GAMESTATE_MENU already loaded, skipping...");
		return;
	}
	switch (game->loadstate) {
		PRELOAD_STATE(GAMESTATE_MENU, Menu)
		PRELOAD_STATE(GAMESTATE_LOADING, Loading)
		PRELOAD_STATE(GAMESTATE_ABOUT, About)
		PRELOAD_STATE(GAMESTATE_INTRO, Intro)
		PRELOAD_STATE(GAMESTATE_MAP, Map)
		PRELOAD_STATE(GAMESTATE_LEVEL, Level)
		default:
			PrintConsole(game, "ERROR: Attempted to preload unknown gamestate %d!", game->loadstate);
			break;
	}
	PrintConsole(game, "finished");
}

void UnloadGameState(struct Game *game) {
	switch (game->gamestate) {
		case GAMESTATE_MENU:
			if (game->shuttingdown) {
				PrintConsole(game, "Unload GAMESTATE_MENU..."); Menu_Unload(game);
			} else {
				PrintConsole(game, "Just stopping GAMESTATE_MENU..."); Menu_Stop(game);
			}
			break;
		UNLOAD_STATE(GAMESTATE_PAUSE, Pause)
		UNLOAD_STATE(GAMESTATE_LOADING, Loading)
		UNLOAD_STATE(GAMESTATE_ABOUT, About)
		UNLOAD_STATE(GAMESTATE_INTRO, Intro)
		UNLOAD_STATE(GAMESTATE_MAP, Map)
		UNLOAD_STATE(GAMESTATE_LEVEL, Level)
		default:
			PrintConsole(game, "ERROR: Attempted to unload unknown gamestate %d!", game->gamestate);
			break;
	}
	PrintConsole(game, "finished");
}

void LoadGameState(struct Game *game) {
	switch (game->loadstate) {
		LOAD_STATE(GAMESTATE_MENU, Menu)
		LOAD_STATE(GAMESTATE_LOADING, Loading)
		LOAD_STATE(GAMESTATE_ABOUT, About)
		LOAD_STATE(GAMESTATE_INTRO, Intro)
		LOAD_STATE(GAMESTATE_MAP, Map)
		LOAD_STATE(GAMESTATE_LEVEL, Level)
		default:
			PrintConsole(game, "ERROR: Attempted to load unknown gamestate %d!", game->loadstate);
	}
	PrintConsole(game, "finished");
	game->gamestate = game->loadstate;
	game->loadstate = -1;
}

void DrawGameState(struct Game *game) {
	switch (game->gamestate) {
		DRAW_STATE(GAMESTATE_MENU, Menu)
		DRAW_STATE(GAMESTATE_PAUSE, Pause)
		DRAW_STATE(GAMESTATE_LOADING, Loading)
		DRAW_STATE(GAMESTATE_ABOUT, About)
		DRAW_STATE(GAMESTATE_INTRO, Intro)
		DRAW_STATE(GAMESTATE_MAP, Map)
		DRAW_STATE(GAMESTATE_LEVEL, Level)
		default:
			game->showconsole = true;
			PrintConsole(game, "ERROR: Unknown gamestate %d reached! (5 sec sleep)", game->gamestate);
			DrawConsole(game);
			al_flip_display();
			al_rest(5.0);
			PrintConsole(game, "Returning to menu...");
			game->gamestate = GAMESTATE_LOADING;
			game->loadstate = GAMESTATE_MENU;
			break;
	}
}

void ScaleBitmap(ALLEGRO_BITMAP* source, int width, int height) {
	if ((al_get_bitmap_width(source)==width) && (al_get_bitmap_height(source)==height)) {
		al_draw_bitmap(source, 0, 0, 0);
		return;
	}
	int x, y;
	al_lock_bitmap(al_get_target_bitmap(), ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
	al_lock_bitmap(source, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);

	/* linear filtering code written by SiegeLord */

	ALLEGRO_COLOR interpolate(ALLEGRO_COLOR c1, ALLEGRO_COLOR c2, float frac) {
		return al_map_rgba_f(c1.r + frac * (c2.r - c1.r),
			c1.g + frac * (c2.g - c1.g),
			c1.b + frac * (c2.b - c1.b),
			c1.a + frac * (c2.a - c1.a));
	}

	for (y = 0; y < height; y++) {
		float pixy = ((float)y / height) * ((float)al_get_bitmap_height(source) - 1);
		float pixy_f = floor(pixy);
		for (x = 0; x < width; x++) {
			float pixx = ((float)x / width) * ((float)al_get_bitmap_width(source) - 1);
			float pixx_f = floor(pixx);

			ALLEGRO_COLOR a = al_get_pixel(source, pixx_f, pixy_f);
			ALLEGRO_COLOR b = al_get_pixel(source, pixx_f + 1, pixy_f);
			ALLEGRO_COLOR c = al_get_pixel(source, pixx_f, pixy_f + 1);
			ALLEGRO_COLOR d = al_get_pixel(source, pixx_f + 1, pixy_f + 1);

			ALLEGRO_COLOR ab = interpolate(a, b, pixx - pixx_f);
			ALLEGRO_COLOR cd = interpolate(c, d, pixx - pixx_f);
			ALLEGRO_COLOR result = interpolate(ab, cd, pixy - pixy_f);

			al_put_pixel(x, y, result);
		}
	}
	al_unlock_bitmap(al_get_target_bitmap());
	al_unlock_bitmap(source);
}

ALLEGRO_BITMAP* LoadScaledBitmap(char* filename, int width, int height) {
	ALLEGRO_BITMAP *source, *target = al_create_bitmap(width, height);
	al_set_target_bitmap(target);
	al_clear_to_color(al_map_rgba(0,0,0,0));
	char origfn[255] = "data/";
	/*char cachefn[255] = "data/cache/";*/
	strcat(origfn, filename);
	/*strcat(cachefn, filename);*/
	void GenerateBitmap() {
		al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);

		source = al_load_bitmap( origfn );
		al_set_new_bitmap_flags(ALLEGRO_MAG_LINEAR | ALLEGRO_MIN_LINEAR);

		ScaleBitmap(source, width, height);
		/*al_save_bitmap(cachefn, target);
		PrintConsole(game, "Cache bitmap %s generated.", filename);*/
		al_destroy_bitmap(source);
	}

	/*source = al_load_bitmap( cachefn );
	if (source) {
		if ((al_get_bitmap_width(source)!=width) || (al_get_bitmap_height(source)!=height)) {
			al_destroy_bitmap(source);*/
			GenerateBitmap();
			return target;
	/*	}
		return source;
	} else GenerateBitmap();
	return target;*/
}

float tps(struct Game *game, float t) {
	if (game->fps>0) return t/game->fps;
	else return t/fps;
}

int Shared_Load(struct Game *game) {
	game->font = al_load_ttf_font("data/ShadowsIntoLight.ttf",al_get_display_height(game->display)*0.09,0 );
	if(!game->font) {
		fprintf(stderr, "failed to load game font!\n");
		return -1;
	}
	game->font_console = al_load_ttf_font("data/DejaVuSansMono.ttf",al_get_display_height(game->display)*0.018,0 );
	if(!game->font_console) {
		fprintf(stderr, "failed to load console font!\n");
		return -1;
	}
	game->console = al_create_bitmap(al_get_display_width(game->display), al_get_display_height(game->display)*0.12);
	al_set_target_bitmap(game->console);
	al_clear_to_color(al_map_rgba(0,0,0,80));
	al_set_target_bitmap(al_get_backbuffer(game->display));
	return 0;
}

void Shared_Unload(struct Game *game) {
	al_destroy_font(game->font);
	al_destroy_font(game->font_console);
	al_destroy_bitmap(game->console);
}

int main(int argc, char **argv){
	srand(time(NULL));

	InitConfig();

	bool redraw = true;

	struct Game game;

	game.fullscreen = atoi(GetConfigOptionDefault("SuperDerpy", "fullscreen", "1"));
	game.music = atoi(GetConfigOptionDefault("SuperDerpy", "music", "10"));
	game.voice = atoi(GetConfigOptionDefault("SuperDerpy", "voice", "10"));
	game.fx = atoi(GetConfigOptionDefault("SuperDerpy", "fx", "10"));
	game.fps = atoi(GetConfigOptionDefault("SuperDerpy", "fps", "0"));
	game.debug = atoi(GetConfigOptionDefault("SuperDerpy", "debug", "0"));
	game.width = atoi(GetConfigOptionDefault("SuperDerpy", "width", "800"));
	if (game.width<320) game.width=320;
	game.height = atoi(GetConfigOptionDefault("SuperDerpy", "height", "500"));
	if (game.height<200) game.height=200;

	if(!al_init()) {
		fprintf(stderr, "failed to initialize allegro!\n");
		return -1;
	}


	if(!al_init_image_addon()) {
		fprintf(stderr, "failed to initialize image addon!\n");
		/*al_show_native_message_box(display, "Error", "Error", "Failed to initialize al_init_image_addon!",
		                           NULL, ALLEGRO_MESSAGEBOX_ERROR);*/
		return 0;
	}

	if(!al_init_acodec_addon()){
		fprintf(stderr, "failed to initialize audio codecs!\n");
		return -1;
	}

	if(!al_install_audio()){
		fprintf(stderr, "failed to initialize audio!\n");
		return -1;
	}

	if(!al_install_keyboard()){
		fprintf(stderr, "failed to initialize keyboard!\n");
		return -1;
	}

/*	if (!al_reserve_samples(10)){
		fprintf(stderr, "failed to reserve samples!\n");
		return -1;
	}
 */
	al_init_font_addon();

	if(!al_init_ttf_addon()){
		fprintf(stderr, "failed to initialize fonts!\n");
		return -1;
	}

	if (game.fullscreen) al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
	else al_set_new_display_flags(ALLEGRO_WINDOWED);
	al_set_new_display_option(ALLEGRO_VSYNC, atoi(GetConfigOptionDefault("SuperDerpy", "vsync", "1")), ALLEGRO_SUGGEST);
	al_set_new_display_option(ALLEGRO_OPENGL, atoi(GetConfigOptionDefault("SuperDerpy", "opengl", "1")), ALLEGRO_SUGGEST);
	game.display = al_create_display(game.width, game.height);
	if(!game.display) {
		fprintf(stderr, "failed to create display!\n");
		return -1;
	}
	al_set_window_title(game.display, "Super Derpy: Muffin Attack");
	if (game.fullscreen) al_hide_mouse_cursor(game.display);
	al_inhibit_screensaver(true);

	al_set_new_bitmap_flags(ALLEGRO_MAG_LINEAR | ALLEGRO_MIN_LINEAR);

	int ret = Shared_Load(&game);
	if (ret!=0) return ret;

	game.event_queue = al_create_event_queue();
	if(!game.event_queue) {
		fprintf(stderr, "failed to create event_queue!\n");
		al_destroy_display(game.display);
		return -1;
	}

	game.audio.v = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_2);
	game.audio.mixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
	game.audio.fx = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
	game.audio.music = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
	game.audio.voice = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
	al_attach_mixer_to_voice(game.audio.mixer, game.audio.v);
	al_attach_mixer_to_mixer(game.audio.fx, game.audio.mixer);
	al_attach_mixer_to_mixer(game.audio.music, game.audio.mixer);
	al_attach_mixer_to_mixer(game.audio.voice, game.audio.mixer);
	al_set_mixer_gain(game.audio.fx, game.fx/10.0);
	al_set_mixer_gain(game.audio.music, game.music/10.0);
	al_set_mixer_gain(game.audio.voice, game.voice/10.0);

	al_register_event_source(game.event_queue, al_get_display_event_source(game.display));
	al_register_event_source(game.event_queue, al_get_keyboard_event_source());

	game.showconsole = game.debug;

	ALLEGRO_DISPLAY_MODE mode;
	al_get_display_mode(0, &mode);
	if (mode.refresh_rate < game.fps) {
		if (atoi(GetConfigOptionDefault("SuperDerpy", "lower_fps_to_refresh_rate", "1"))) {
			PrintConsole(&game, "Refresh rate %d lower than FPS %d, lowering", mode.refresh_rate, game.fps);
			game.fps = mode.refresh_rate;
		} else {
			PrintConsole(&game, "Refresh rate %d lower than FPS %d, NOT lowering due to config", mode.refresh_rate, game.fps);
		}
	} else if (game.fps == 0) game.fps = mode.refresh_rate;
	if (game.fps>600) game.fps = 600;
	al_clear_to_color(al_map_rgb(0,0,0));
	al_flip_display();

	if (game.fps>0) game.timer = al_create_timer(ALLEGRO_BPS_TO_SECS(game.fps));
	else game.timer = al_create_timer(ALLEGRO_BPS_TO_SECS(600));
	if(!game.timer) {
		fprintf(stderr, "failed to create timer!\n");
		return -1;
	}
	al_register_event_source(game.event_queue, al_get_timer_event_source(game.timer));
	al_start_timer(game.timer);

	setlocale(LC_NUMERIC, "C"); //FIXME?

	game.shuttingdown = false;
	game.menu.loaded = false;
	game.restart = false;
	game.loadstate = GAMESTATE_LOADING;
	PreloadGameState(&game);
	LoadGameState(&game);
	game.loadstate = GAMESTATE_MENU;

	int c;
	while ((c = getopt (argc, argv, "l:")) != -1)
		switch (c) {
			case 'l':
				game.level.current_level = optarg[0]-'0';
				game.loadstate = GAMESTATE_LEVEL;
				break;
		}

	while(1) {
		ALLEGRO_EVENT ev;
		bool event = false;
		if (game.fps<0) {
			redraw = true;
			event=al_get_next_event(game.event_queue, &ev);
		} else {
			al_wait_for_event(game.event_queue, &ev);
			event=true;
		}
		if (!event) {}
		else if ((ev.type == ALLEGRO_EVENT_TIMER) && (ev.timer.source == game.timer)) {
			redraw = true;
		}
		else if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
			break;
		}
		else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
			/*PrintConsole(&game, "KEYCODE: %s", al_keycode_to_name(ev.keyboard.keycode));*/
			#ifdef ALLEGRO_MACOSX
			if ((ev.type == ALLEGRO_EVENT_KEY_DOWN) && (ev.keyboard.keycode == 104)) {
			#else
			if ((ev.type == ALLEGRO_EVENT_KEY_DOWN) && (ev.keyboard.keycode == ALLEGRO_KEY_TILDE)) {
			#endif
				game.showconsole = !game.showconsole;
			}
			else if ((game.debug) && (ev.type == ALLEGRO_EVENT_KEY_DOWN) && (ev.keyboard.keycode == ALLEGRO_KEY_F1)) {
				int i;
				for (i=0; i<512; i++) {
					DrawGameState(&game);
				}
				game.showconsole = true;
				PrintConsole(&game, "DEBUG: 512 frames skipped...");
			}
			KEYDOWN_STATE(GAMESTATE_PAUSE, Pause)
			KEYDOWN_STATE(GAMESTATE_MENU, Menu)
			KEYDOWN_STATE(GAMESTATE_LOADING, Loading)
			KEYDOWN_STATE(GAMESTATE_ABOUT, About)
			KEYDOWN_STATE(GAMESTATE_INTRO, Intro)
			KEYDOWN_STATE(GAMESTATE_MAP, Map)
			KEYDOWN_STATE(GAMESTATE_LEVEL, Level)
			else {
				game.showconsole = true;
				PrintConsole(&game, "ERROR: Keystroke in unknown (%d) gamestate! (5 sec sleep)", game.gamestate);
				DrawConsole(&game);
				al_flip_display();
				al_rest(5.0);
				PrintConsole(&game, "Returning to menu...");
				game.gamestate = GAMESTATE_LOADING;
				game.loadstate = GAMESTATE_MENU;
			}
		} else {
			if (game.gamestate == GAMESTATE_LEVEL) {
				Level_ProcessLogic(&game, &ev);
			}
		}

		if(redraw && al_is_event_queue_empty(game.event_queue)) {
			redraw = false;
			DrawGameState(&game);
			DrawConsole(&game);
			al_flip_display();
		}
	}
	game.shuttingdown = true;
	UnloadGameState(&game);
	if (game.gamestate != GAMESTATE_LOADING) {
		game.gamestate = GAMESTATE_LOADING;
		UnloadGameState(&game);
	}
	al_clear_to_color(al_map_rgb(0,0,0));
	PrintConsole(&game, "Shutting down...");
	DrawConsole(&game);
	al_flip_display();
	al_rest(0.1);
	al_destroy_timer(game.timer);
	Shared_Unload(&game);
	al_destroy_display(game.display);
	al_destroy_event_queue(game.event_queue);
	al_destroy_mixer(game.audio.fx);
	al_destroy_mixer(game.audio.music);
	al_destroy_mixer(game.audio.mixer);
	al_destroy_voice(game.audio.v);
	al_uninstall_audio();
	DeinitConfig();
	if (game.restart) {
		al_shutdown_ttf_addon();
		al_shutdown_font_addon();
		#ifdef ALLEGRO_MACOSX
			return _al_mangled_main(argc, argv);
		#else
			return main(argc, argv);
		#endif
	}
	return 0;
}
