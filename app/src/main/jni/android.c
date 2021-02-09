/*
 * android.c: Android front end for my puzzle collection.
 */

#include <jni.h>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>
#include "puzzles.h"

#ifndef JNICALL
#define JNICALL
#endif
#ifndef JNIEXPORT
#define JNIEXPORT
#endif

const struct game* thegame;

void fatal(const char *fmt, ...)
{
	va_list ap;
	fprintf(stderr, "fatal error: ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	exit(1);
}

struct frontend {
	midend *me;
	int timer_active;
	struct timeval last_time;
	config_item *cfg;
	int cfg_which;
	int ox, oy;
};

static frontend *fe = NULL;
static pthread_key_t envKey;
static jobject obj = NULL;

static jobject gameView = NULL;
static jmethodID
	blitterAlloc,
	blitterFree,
	blitterLoad,
	blitterSave,
	clipRect,
	dialogAddString,
	dialogAddBoolean,
	dialogAddChoices,
	dialogInit,
	dialogShow,
	drawCircle,
	drawLine,
	drawPoly,
	drawText,
	fillRect,
	getBackgroundColour,
	postInvalidate,
	requestTimer,
	serialiseWrite,
	setStatus,
	unClip,
	setKeys;

void throwIllegalArgumentException(JNIEnv *env, const char* reason) {
	jclass exCls = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
	(*env)->ThrowNew(env, exCls, reason);
	(*env)->DeleteLocalRef(env, exCls);
}

void get_random_seed(void **randseed, int *randseedsize)
{
	struct timeval *tvp = snew(struct timeval);
	gettimeofday(tvp, NULL);
	*randseed = (void *)tvp;
	*randseedsize = sizeof(struct timeval);
}

void frontend_default_colour(frontend *fe, float *output)
{
	JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
	jint argb = (*env)->CallIntMethod(env, gameView, getBackgroundColour);
	output[0] = ((argb & 0x00ff0000) >> 16) / 255.0f;
	output[1] = ((argb & 0x0000ff00) >> 8) / 255.0f;
	output[2] = (argb & 0x000000ff) / 255.0f;
}

void android_status_bar(void *handle, const char *text)
{
	JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
	jstring js = (*env)->NewStringUTF(env, text);
	if( js == NULL ) return;
	(*env)->CallVoidMethod(env, obj, setStatus, js);
	(*env)->DeleteLocalRef(env, js);
}

#define CHECK_DR_HANDLE if ((frontend*)handle != fe) return;

void android_start_draw(void *handle)
{
	CHECK_DR_HANDLE
//	JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
}

void android_clip(void *handle, int x, int y, int w, int h)
{
	CHECK_DR_HANDLE
	JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
	(*env)->CallVoidMethod(env, gameView, clipRect, x + fe->ox, y + fe->oy, w, h);
}

void android_unclip(void *handle)
{
	CHECK_DR_HANDLE
	JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
	(*env)->CallVoidMethod(env, gameView, unClip, fe->ox, fe->oy);
}

void android_draw_text(void *handle, int x, int y, int fonttype, int fontsize,
		int align, int colour, const char *text)
{
	CHECK_DR_HANDLE
	JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
	jstring js = (*env)->NewStringUTF(env, text);
	if( js == NULL ) return;
	(*env)->CallVoidMethod(env, gameView, drawText, x + fe->ox, y + fe->oy,
			(fonttype == FONT_FIXED ? 0x10 : 0x0) | align,
			fontsize, colour, js);
	(*env)->DeleteLocalRef(env, js);
}

void android_draw_rect(void *handle, int x, int y, int w, int h, int colour)
{
	CHECK_DR_HANDLE
	JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
	(*env)->CallVoidMethod(env, gameView, fillRect, x + fe->ox, y + fe->oy, w, h, colour);
}

void android_draw_thick_line(void *handle, float thickness, float x1, float y1, float x2, float y2, int colour)
{
	CHECK_DR_HANDLE
	JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
	(*env)->CallVoidMethod(env, gameView, drawLine, thickness, x1 + fe->ox, y1 + fe->oy, x2 + fe->ox, y2 + fe->oy, colour);
}

void android_draw_line(void *handle, int x1, int y1, int x2, int y2, int colour)
{
	android_draw_thick_line(handle, 1.f, x1, y1, x2, y2, colour);
}

void android_draw_thick_poly(void *handle, float thickness, int *coords, int npoints,
		int fillcolour, int outlinecolour)
{
	CHECK_DR_HANDLE
	JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
	jintArray coordsJava = (*env)->NewIntArray(env, npoints*2);
	if (coordsJava == NULL) return;
	(*env)->SetIntArrayRegion(env, coordsJava, 0, npoints*2, coords);
	(*env)->CallVoidMethod(env, gameView, drawPoly, thickness, coordsJava, fe->ox, fe->oy, outlinecolour, fillcolour);
	(*env)->DeleteLocalRef(env, coordsJava);  // prevent ref table exhaustion on e.g. large Mines grids...
}

void android_draw_poly(void *handle, int *coords, int npoints,
		int fillcolour, int outlinecolour)
{
	android_draw_thick_poly(handle, 1.f, coords, npoints, fillcolour, outlinecolour);
}

void android_draw_thick_circle(void *handle, float thickness, float cx, float cy, float radius, int fillcolour, int outlinecolour)
{
	CHECK_DR_HANDLE
	JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
	(*env)->CallVoidMethod(env, gameView, drawCircle, thickness, cx+fe->ox, cy+fe->oy, radius, outlinecolour, fillcolour);
}

void android_draw_circle(void *handle, int cx, int cy, int radius, int fillcolour, int outlinecolour)
{
	android_draw_thick_circle(handle, 1.f, cx, cy, radius, fillcolour, outlinecolour);
}

struct blitter {
	int handle, w, h, x, y;
};

blitter *android_blitter_new(void *handle, int w, int h)
{
	blitter *bl = snew(blitter);
	bl->handle = -1;
	bl->w = w;
	bl->h = h;
	return bl;
}

void android_blitter_free(void *handle, blitter *bl)
{
	if (bl->handle != -1) {
		JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
		(*env)->CallVoidMethod(env, gameView, blitterFree, bl->handle);
	}
	sfree(bl);
}

void android_blitter_save(void *handle, blitter *bl, int x, int y)
{
	CHECK_DR_HANDLE
	JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
	if (bl->handle == -1)
		bl->handle = (*env)->CallIntMethod(env, gameView, blitterAlloc, bl->w, bl->h);
	bl->x = x;
	bl->y = y;
	(*env)->CallVoidMethod(env, gameView, blitterSave, bl->handle, x + fe->ox, y + fe->oy);
}

void android_blitter_load(void *handle, blitter *bl, int x, int y)
{
	CHECK_DR_HANDLE
	assert(bl->handle != -1);
	if (x == BLITTER_FROMSAVED && y == BLITTER_FROMSAVED) {
		x = bl->x;
		y = bl->y;
	}
	JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
	(*env)->CallVoidMethod(env, gameView, blitterLoad, bl->handle, x + fe->ox, y + fe->oy);
}

void android_end_draw(void *handle)
{
	JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
	(*env)->CallVoidMethod(env, gameView, postInvalidate);
}

static char *android_text_fallback(void *handle, const char *const *strings,
			       int nstrings)
{
    /*
     * We assume Android can cope with any UTF-8 likely to be emitted
     * by a puzzle.
     */
    return dupstr(strings[0]);
}

const struct drawing_api android_drawing = {
	android_draw_text,
	android_draw_rect,
	android_draw_line,
	android_draw_poly,
	android_draw_circle,
	NULL, // draw_update,
	android_clip,
	android_unclip,
	android_start_draw,
	android_end_draw,
	android_status_bar,
	android_blitter_new,
	android_blitter_free,
	android_blitter_save,
	android_blitter_load,
	NULL, NULL, NULL, NULL, NULL, NULL, /* {begin,end}_{doc,page,puzzle} */
	NULL, NULL,				   /* line_width, line_dotted */
	android_text_fallback,
	android_draw_thick_line,
};

void JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_keyEvent(JNIEnv *env, jobject _obj, jint x, jint y, jint keyval)
{
	pthread_setspecific(envKey, env);
	if (fe->ox == -1 || keyval < 0) return;
	midend_process_key(fe->me, x - fe->ox, y - fe->oy, keyval);
}

jfloat JNICALL Java_name_boyle_chris_sgtpuzzles_GameView_suggestDensity(JNIEnv *env, jobject _view, jint viewWidth, jint viewHeight)
{
	if (!fe || !fe->me) return 1.f;
	pthread_setspecific(envKey, env);
	int defaultW = INT_MAX, defaultH = INT_MAX;
	midend_reset_tilesize(fe->me);
	midend_size(fe->me, &defaultW, &defaultH, false);
	return max(1.f, min(floor(((float)viewWidth) / defaultW), floor(((float)viewHeight) / defaultH)));
}

void JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_resizeEvent(JNIEnv *env, jobject _obj, jint viewWidth, jint viewHeight)
{
	pthread_setspecific(envKey, env);
	if (!fe || !fe->me) return;
	int w = viewWidth, h = viewHeight;
	midend_size(fe->me, &w, &h, true);
	fe->ox = (viewWidth - w) / 2;
	fe->oy = (viewHeight - h) / 2;
	if (gameView) (*env)->CallVoidMethod(env, gameView, unClip, fe->ox, fe->oy);
	midend_force_redraw(fe->me);
}

void JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_timerTick(JNIEnv *env, jobject _obj)
{
	if (! fe->timer_active) return;
	pthread_setspecific(envKey, env);
	struct timeval now;
	float elapsed;
	gettimeofday(&now, NULL);
	elapsed = ((now.tv_usec - fe->last_time.tv_usec) * 0.000001F +
			(now.tv_sec - fe->last_time.tv_sec));
		midend_timer(fe->me, elapsed);  // may clear timer_active
	fe->last_time = now;
}

void deactivate_timer(frontend *_fe)
{
	if (!fe) return;
	if (fe->timer_active) {
		JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
		(*env)->CallVoidMethod(env, obj, requestTimer, false);
	}
	fe->timer_active = false;
}

void activate_timer(frontend *_fe)
{
	if (!fe) return;
	if (!fe->timer_active) {
		JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
		(*env)->CallVoidMethod(env, obj, requestTimer, true);
		gettimeofday(&fe->last_time, NULL);
	}
	fe->timer_active = true;
}

void JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_resetTimerBaseline(JNIEnv *env, jobject _obj)
{
	if (!fe) return;
	gettimeofday(&fe->last_time, NULL);
}

config_item* configItemWithName(JNIEnv *env, jstring js)
{
	const char* name = (*env)->GetStringUTFChars(env, js, NULL);
	config_item* i;
	config_item* ret = NULL;
	for (i = fe->cfg; i->type != C_END; i++) {
		if (!strcmp(name, i->name)) {
			ret = i;
			break;
		}
	}
	(*env)->ReleaseStringUTFChars(env, js, name);
	return ret;
}

void JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_configSetString(JNIEnv *env, jobject _obj, jstring name, jstring s)
{
	pthread_setspecific(envKey, env);
	config_item *i = configItemWithName(env, name);
	const char* newval = (*env)->GetStringUTFChars(env, s, NULL);
	sfree(i->u.string.sval);
	i->u.string.sval = dupstr(newval);
	(*env)->ReleaseStringUTFChars(env, s, newval);
}

void JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_configSetBool(JNIEnv *env, jobject _obj, jstring name, jint selected)
{
	pthread_setspecific(envKey, env);
	config_item *i = configItemWithName(env, name);
	i->u.boolean.bval = selected != 0 ? true : false;
}

void JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_configSetChoice(JNIEnv *env, jobject _obj, jstring name, jint selected)
{
	pthread_setspecific(envKey, env);
	config_item *i = configItemWithName(env, name);
	i->u.choices.selected = selected;
}

void JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_solveEvent(JNIEnv *env, jobject _obj)
{
	pthread_setspecific(envKey, env);
	const char *msg = midend_solve(fe->me);
	if (! msg) return;
	jstring js = (*env)->NewStringUTF(env, msg);
	if( js == NULL ) return;
	throwIllegalArgumentException(env, msg);
}

void JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_restartEvent(JNIEnv *env, jobject _obj)
{
	pthread_setspecific(envKey, env);
	midend_restart_game(fe->me);
}

void JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_configEvent(JNIEnv *env, jobject _obj, jint whichEvent)
{
	pthread_setspecific(envKey, env);
	char *title;
	config_item *i;
	fe->cfg = midend_get_config(fe->me, whichEvent, &title);
	fe->cfg_which = whichEvent;
	jstring js = (*env)->NewStringUTF(env, title);
	if( js == NULL ) return;
	(*env)->CallVoidMethod(env, obj, dialogInit, whichEvent, js);
	for (i = fe->cfg; i->type != C_END; i++) {
		jstring name = NULL;
		if (i->name) {
			name = (*env)->NewStringUTF(env, i->name);
			if (!name) return;
		}
		jstring sval = NULL;
		switch (i->type) {
			case C_STRING:
				if (i->u.string.sval) {
					sval = (*env)->NewStringUTF(env, i->u.string.sval);
					if (!sval) return;
				}
				(*env)->CallVoidMethod(env, obj, dialogAddString, whichEvent, name, sval);
				break;
			case C_CHOICES:
				if (i->u.choices.choicenames) {
					sval = (*env)->NewStringUTF(env, i->u.choices.choicenames);
					if (!sval) return;
				}
				(*env)->CallVoidMethod(env, obj, dialogAddChoices, whichEvent, name, sval, i->u.choices.selected);
				break;
			case C_BOOLEAN: case C_END: default:
				(*env)->CallVoidMethod(env, obj, dialogAddBoolean, whichEvent, name, i->u.boolean.bval);
				break;
		}
		if (name) (*env)->DeleteLocalRef(env, name);
		if (sval) (*env)->DeleteLocalRef(env, sval);
	}
	(*env)->CallVoidMethod(env, obj, dialogShow);
}

jstring JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_configOK(JNIEnv *env, jobject _obj)
{
	pthread_setspecific(envKey, env);
    const char *err = midend_set_config(fe->me, CFG_SETTINGS, fe->cfg);
    if (err) {
		throwIllegalArgumentException(env, err);
		return NULL;
	}
	char *encoded = thegame->encode_params(thegame->custom_params(fe->cfg), true);
	free_cfg(fe->cfg);
	fe->cfg = NULL;

	jstring ret = (*env)->NewStringUTF(env, encoded);
	sfree(encoded);
	return ret;
}

char *get_current_params(int mode)
{
	char sep = (mode == CFG_SEED) ? (char)'#' : (char)':';
	char *wintitle;
	char *current_cfg = midend_get_config(fe->me, mode, &wintitle)->u.string.sval;
	sfree(wintitle);
	size_t plen = strchr(current_cfg, sep) - current_cfg + 1;
	char *params = snewn(plen, char);
	strncpy(params, current_cfg,plen - 1);
	sfree(current_cfg);
	params[plen - 1] = '\0';
	return params;
}

jstring getDescOrSeedFromDialog(JNIEnv *env, jobject _obj, int mode)
{
	/* we must build a fully-specified string (with params) so GameLaunch knows params,
	   and in the case of seed, so the game gen process generates with correct params */
	pthread_setspecific(envKey, env);
	char sep = (mode == CFG_SEED) ? (char)'#' : (char)':';
	char *buf;
	int free_buf = false;
	jstring ret = NULL;
	if (!strchr(fe->cfg[0].u.string.sval, sep)) {
		char *params = get_current_params(mode);
		size_t plen = strlen(params);
		buf = snewn(plen + strlen(fe->cfg[0].u.string.sval) + 2, char);
		sprintf(buf, "%s%c%s", params, sep, fe->cfg[0].u.string.sval);
		sfree(params);
		free_buf = true;
	} else {
		buf = fe->cfg[0].u.string.sval;
	}
	char *willBeMangled = dupstr(buf);
	const char *error = midend_game_id(fe->me, willBeMangled);
	sfree(willBeMangled);
	if (!error) ret = (*env)->NewStringUTF(env, buf);
	if (free_buf) sfree(buf);
	if (error) {
		throwIllegalArgumentException(env, error);
	} else {
		free_cfg(fe->cfg);
		fe->cfg = NULL;
	}
	return ret;
}

jstring JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_getFullGameIDFromDialog(JNIEnv *env, jobject _obj)
{
	return getDescOrSeedFromDialog(env, _obj, CFG_DESC);
}

jstring JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_getFullSeedFromDialog(JNIEnv *env, jobject _obj)
{
	return getDescOrSeedFromDialog(env, _obj, CFG_SEED);
}

void JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_configCancel(JNIEnv *env, jobject _obj)
{
	pthread_setspecific(envKey, env);
	free_cfg(fe->cfg);
	fe->cfg = NULL;
}

void android_serialise_write(void *ctx, const void *buf, int len)
{
	JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
	jbyteArray bytesJava = (*env)->NewByteArray(env, len);
	if (bytesJava == NULL) return;
	(*env)->SetByteArrayRegion(env, bytesJava, 0, len, buf);
	(*env)->CallVoidMethod(env, obj, serialiseWrite, bytesJava);
	(*env)->DeleteLocalRef(env, bytesJava);
}

void JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_serialise(JNIEnv *env, jobject _obj)
{
	if (!fe) return;
	pthread_setspecific(envKey, env);
	midend_serialise(fe->me, android_serialise_write, (void*)0);
}

static const char* deserialise_readptr = NULL;
static size_t deserialise_readlen = 0;

bool android_deserialise_read(void *ctx, void *buf, int len)
{
	if (len < 0) return false;
	size_t l = min((size_t)len, deserialise_readlen);
	if (l == 0) return len == 0;
	memcpy(buf, deserialise_readptr, l);
	deserialise_readptr += l;
	deserialise_readlen -= l;
	return l == len;
}

int deserialiseOrIdentify(frontend *new_fe, jstring s, jboolean identifyOnly) {
	JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
	const char * c = (*env)->GetStringUTFChars(env, s, NULL);
	deserialise_readptr = c;
	deserialise_readlen = strlen(deserialise_readptr);
	char *name;
	const char *error = identify_game(&name, android_deserialise_read, NULL);
	int whichBackend = -1;
	if (! error) {
		int i;
		for (i = 0; i < gamecount; i++) {
			if (!strcmp(gamelist[i]->name, name)) {
				whichBackend = i;
			}
		}
		if (whichBackend < 0) error = "Internal error identifying game";
	}
	if (! error && ! identifyOnly) {
		thegame = gamelist[whichBackend];
		new_fe->me = midend_new(new_fe, gamelist[whichBackend], &android_drawing, new_fe);
		deserialise_readptr = c;
		deserialise_readlen = strlen(deserialise_readptr);
		error = midend_deserialise(new_fe->me, android_deserialise_read, NULL);
	}
	(*env)->ReleaseStringUTFChars(env, s, c);
	if (error) {
		throwIllegalArgumentException(env, error);
		if (!identifyOnly && new_fe->me) {
			midend_free(new_fe->me);
			new_fe->me = NULL;
		}
	}
	return whichBackend;
}

jint JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_identifyBackend(JNIEnv *env, jclass type, jstring savedGame)
{
	pthread_setspecific(envKey, env);
	return deserialiseOrIdentify(NULL, savedGame, true);
}

jstring JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_getCurrentParams(JNIEnv *env, jobject _obj)
{
	if (! fe || ! fe->me) return NULL;
	char *params = get_current_params(CFG_SEED);
	jstring ret = (*env)->NewStringUTF(env, params);
	sfree(params);
	return ret;
}

jstring JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_htmlHelpTopic(JNIEnv *env, jobject _obj)
{
	//pthread_setspecific(envKey, env);
	return (*env)->NewStringUTF(env, thegame->htmlhelp_topic);
}

const game* game_by_name(const char* name) {
	for (int i = 0; i<gamecount; i++)
	    if (!strcmp(name, gamelist[i]->htmlhelp_topic)) return gamelist[i];
	return NULL;
}

game_params* oriented_params_from_str(const game* my_game, const char* params_str, const char** error) {
	game_params *params = my_game->default_params();
	if (params_str != NULL) {
		if (!strcmp(params_str, "--portrait") || !strcmp(params_str, "--landscape")) {
			unsigned int w, h;
			int pos;
			char * encoded = my_game->encode_params(params, true);
			if (sscanf(encoded, "%ux%u%n", &w, &h, &pos) >= 2) {
				if ((w > h) != (params_str[2] == 'l')) {
					sprintf(encoded, "%ux%u%s", h, w, encoded + pos);
					my_game->decode_params(params, encoded);
				}
			}
			sfree(encoded);
		} else {
			my_game->decode_params(params, params_str);
		}
	}
	const char *our_error = my_game->validate_params(params, true);
	if (our_error) {
		my_game->free_params(params);
		if (error) {
			(*error) = our_error;
		}
		return NULL;
	}
	return params;
}

void startPlayingIntGameID(frontend* new_fe, jstring jsGameID, jstring backend)
{
	JNIEnv *env = (JNIEnv*)pthread_getspecific(envKey);
	const char * backendChars = (*env)->GetStringUTFChars(env, backend, NULL);
	const game * g = game_by_name(backendChars);
	(*env)->ReleaseStringUTFChars(env, backend, backendChars);
	if (!g) {
		throwIllegalArgumentException(env, "Internal error identifying game");
		return;
	}
	thegame = g;
	new_fe->me = midend_new(new_fe, g, &android_drawing, new_fe);
	const char * gameIDjs = (*env)->GetStringUTFChars(env, jsGameID, NULL);
	char * gameID = dupstr(gameIDjs);
	(*env)->ReleaseStringUTFChars(env, jsGameID, gameIDjs);
	const char * error = midend_game_id(new_fe->me, gameID);
	sfree(gameID);
	if (error) {
		throwIllegalArgumentException(env, error);
		return;
	}
	midend_new_game(new_fe->me);
}

jfloatArray JNICALL Java_name_boyle_chris_sgtpuzzles_GameView_getColours(JNIEnv *env, jobject _obj)
{
	int n;
	float* colours;
	colours = midend_colours(fe->me, &n);
	jfloatArray jColours = (*env)->NewFloatArray(env, n*3);
	if (jColours == NULL) return NULL;
	(*env)->SetFloatArrayRegion(env, jColours, 0, n*3, colours);
	return jColours;
}

jobject getPresetInternal(JNIEnv *env, struct preset_menu_entry entry);

jobjectArray getPresetsInternal(JNIEnv *env, struct preset_menu *menu) {
    jclass MenuEntry = (*env)->FindClass(env, "name/boyle/chris/sgtpuzzles/MenuEntry");
    jobjectArray ret = (*env)->NewObjectArray(env, menu->n_entries, MenuEntry, NULL);
    for (int i = 0; i < menu->n_entries; i++) {
        jobject menuItem = getPresetInternal(env, menu->entries[i]);
        (*env)->SetObjectArrayElement(env, ret, i, menuItem);
    }
    return ret;
}

jobject getPresetInternal(JNIEnv *env, const struct preset_menu_entry entry) {
    jclass MenuEntry = (*env)->FindClass(env, "name/boyle/chris/sgtpuzzles/MenuEntry");
    jstring title = (*env)->NewStringUTF(env, entry.title);
    if (entry.submenu) {
        jobject submenu = getPresetsInternal(env, entry.submenu);
        jmethodID newEntryWithSubmenu = (*env)->GetMethodID(env, MenuEntry,  "<init>", "(ILjava/lang/String;[Lname/boyle/chris/sgtpuzzles/MenuEntry;)V");
        return (*env)->NewObject(env, MenuEntry, newEntryWithSubmenu, entry.id, title, submenu);
    } else {
        jstring params = (*env)->NewStringUTF(env, thegame->encode_params(entry.params, true));
        jmethodID newEntryWithParams = (*env)->GetMethodID(env, MenuEntry,  "<init>", "(ILjava/lang/String;Ljava/lang/String;)V");
        return (*env)->NewObject(env, MenuEntry, newEntryWithParams, entry.id, title, params);
    }
}

jobjectArray JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_getPresets(JNIEnv *env, jobject _obj)
{
	struct preset_menu* menu = midend_get_presets(fe->me, NULL);
	return getPresetsInternal(env, menu);
}

jint JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_getUIVisibility(JNIEnv *env, jobject _obj) {
	return (midend_can_undo(fe->me))
			+ (midend_can_redo(fe->me) << 1)
			+ (thegame->can_configure << 2)
			+ (thegame->can_solve << 3)
			+ (midend_wants_statusbar(fe->me) << 4);
}

void requestKeys(JNIEnv *env, frontend* new_fe)
{
    int nkeys = 0;
    const key_label *keys = midend_request_keys(new_fe->me, &nkeys);
    char *keyChars = snewn(nkeys + 1, char);
    int pos = 0;
    for (int i = 0; i < nkeys; i++)
        keyChars[pos++] = (char)keys[i].button;
    keyChars[pos] = '\0';
    jstring jKeys = (*env)->NewStringUTF(env, keyChars);
    (*env)->CallVoidMethod(env, obj, setKeys, jKeys);
    (*env)->DeleteLocalRef(env, jKeys);
    sfree(keyChars);
}

void startPlayingInt(JNIEnv *env, jobject _obj, jobject _gameView, jstring backend, jstring saveOrGameID, int isGameID)
{
	pthread_setspecific(envKey, env);
	if (obj) (*env)->DeleteGlobalRef(env, obj);
	obj = (*env)->NewGlobalRef(env, _obj);
	if (gameView) (*env)->DeleteGlobalRef(env, gameView);
	gameView = (*env)->NewGlobalRef(env, _gameView);

	frontend *new_fe = snew(frontend);
	memset(new_fe, 0, sizeof(frontend));
	new_fe->ox = -1;
	if (isGameID) {
		startPlayingIntGameID(new_fe, saveOrGameID, backend);
	} else {
		deserialiseOrIdentify(new_fe, saveOrGameID, false);
		if ((*env)->ExceptionCheck(env)) return;
	}

	if (fe) {
		if (fe->me) midend_free(fe->me);  // might use gameView (e.g. blitters)
		sfree(fe);
	}
	fe = new_fe;
	int x, y;
	x = INT_MAX;
	y = INT_MAX;
	midend_size(fe->me, &x, &y, false);
    requestKeys(env, fe);
}

void JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_startPlaying(JNIEnv *env, jobject _obj, jobject _gameView, jstring savedGame)
{
	startPlayingInt(env, _obj, _gameView, NULL, savedGame, false);
}

void JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_startPlayingGameID(JNIEnv *env, jobject _obj, jobject _gameView, jstring backend, jstring gameID)
{
	startPlayingInt(env, _obj, _gameView, backend, gameID, true);
}

jboolean JNICALL Java_name_boyle_chris_sgtpuzzles_GamePlay_isCompletedNow(JNIEnv *env, jobject _obj) {
    return fe && fe->me && midend_status(fe->me) ? true : false;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved)
{
	jclass cls, vcls;
	JNIEnv *env;
	if ((*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_6)) return JNI_ERR;
	pthread_key_create(&envKey, NULL);
	pthread_setspecific(envKey, env);
	cls = (*env)->FindClass(env, "name/boyle/chris/sgtpuzzles/GamePlay");
	vcls = (*env)->FindClass(env, "name/boyle/chris/sgtpuzzles/GameView");
	blitterAlloc   = (*env)->GetMethodID(env, vcls, "blitterAlloc", "(II)I");
	blitterFree    = (*env)->GetMethodID(env, vcls, "blitterFree", "(I)V");
	blitterLoad    = (*env)->GetMethodID(env, vcls, "blitterLoad", "(III)V");
	blitterSave    = (*env)->GetMethodID(env, vcls, "blitterSave", "(III)V");
	clipRect       = (*env)->GetMethodID(env, vcls, "clipRect", "(IIII)V");
	dialogAddString = (*env)->GetMethodID(env, cls,  "dialogAddString", "(ILjava/lang/String;Ljava/lang/String;)V");
    dialogAddBoolean = (*env)->GetMethodID(env, cls,  "dialogAddBoolean", "(ILjava/lang/String;Z)V");
    dialogAddChoices = (*env)->GetMethodID(env, cls,  "dialogAddChoices", "(ILjava/lang/String;Ljava/lang/String;I)V");
	dialogInit     = (*env)->GetMethodID(env, cls,  "dialogInit", "(ILjava/lang/String;)V");
	dialogShow     = (*env)->GetMethodID(env, cls,  "dialogShow", "()V");
	drawCircle     = (*env)->GetMethodID(env, vcls, "drawCircle", "(FFFFII)V");
	drawLine       = (*env)->GetMethodID(env, vcls, "drawLine", "(FFFFFI)V");
	drawPoly       = (*env)->GetMethodID(env, vcls,  "drawPoly", "(F[IIIII)V");
	drawText       = (*env)->GetMethodID(env, vcls, "drawText", "(IIIIILjava/lang/String;)V");
	fillRect       = (*env)->GetMethodID(env, vcls, "fillRect", "(IIIII)V");
	getBackgroundColour = (*env)->GetMethodID(env, vcls, "getDefaultBackgroundColour", "()I");
	postInvalidate = (*env)->GetMethodID(env, vcls, "postInvalidate", "()V");
	requestTimer   = (*env)->GetMethodID(env, cls,  "requestTimer", "(Z)V");
	serialiseWrite = (*env)->GetMethodID(env, cls,  "serialiseWrite", "([B)V");
	setStatus      = (*env)->GetMethodID(env, cls,  "setStatus", "(Ljava/lang/String;)V");
	unClip         = (*env)->GetMethodID(env, vcls, "unClip", "(II)V");
	setKeys        = (*env)->GetMethodID(env, cls,  "setKeys","(Ljava/lang/String;)V");

	return JNI_VERSION_1_6;
}
