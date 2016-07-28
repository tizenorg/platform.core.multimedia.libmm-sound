/*
 * libmm-sound
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Seungbae Shin <seungbae.shin@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <semaphore.h>

#include <mm_error.h>
#include <mm_debug.h>
#include <pthread.h>
#include <mm_sound_pa_client.h>

#include "../../include/mm_sound_plugin_codec.h"
#include "../../../include/mm_sound_common.h"
#include <unistd.h>

#include <sndfile.h>
#include <pulse/pulseaudio.h>

typedef struct
{
	int handle;
	int repeat_count;
	int (*stop_cb)(int);
	int cb_param;
	char filename[256];
	char stream_type[MM_SOUND_STREAM_TYPE_LEN];
	int stream_index;

	pa_threaded_mainloop *m;
	pa_context *c;
	pa_stream* s;
	pa_sample_spec spec;
	SNDFILE* sf;
	SF_INFO si;
} wave_info_t;

/* Context Callbacks */
static void _context_state_callback(pa_context *c, void *userdata) {
	pa_threaded_mainloop *m = (pa_threaded_mainloop *)userdata;
	assert(c);

	debug_msg("context state callback : %p, %d", c, pa_context_get_state(c));

	switch (pa_context_get_state(c)) {
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
			break;

		case PA_CONTEXT_READY:
		case PA_CONTEXT_TERMINATED:
		case PA_CONTEXT_FAILED:
			pa_threaded_mainloop_signal(m, 0);
			break;

		default:
			break;
	}
}

static void _context_drain_complete(pa_context *c, void *userdata)
{
	pa_context_disconnect(c);
}

/* Stream Callbacks */
static void _stream_state_callback(pa_stream *s, void *userdata)
{
	pa_threaded_mainloop *m = (pa_threaded_mainloop *)userdata;
	assert(s);

	debug_msg("stream state callback : %p, %d", s, pa_stream_get_state(s));

	switch (pa_stream_get_state(s)) {
		case PA_STREAM_CREATING:
			break;
		case PA_STREAM_READY:
		case PA_STREAM_FAILED:
		case PA_STREAM_TERMINATED:
			pa_threaded_mainloop_signal(m, 0);
			break;
		default:
			break;
	}
}

static void _stream_drain_complete(pa_stream* s, int success, void *userdata)
{
	pa_operation *o;
	wave_info_t *h = (wave_info_t *)userdata;

	debug_msg("stream drain complete callback : %p, %d", s, success);

	if (!success) {
		debug_error("drain failed. s(%p), success(%d)", s, success);
		//pa_threaded_mainloop_signal(h->m, 0);
	}

	pa_stream_disconnect(h->s);
	pa_stream_unref(h->s);
	h->s = NULL;

	if (!(o = pa_context_drain(h->c, _context_drain_complete, h)))
		pa_context_disconnect(h->c);
	else
		pa_operation_unref(o);

	/* send EOS callback */
	debug_msg("Send EOS to CALLBACK(%p, %p) to client!!", h->stop_cb, h->cb_param);
	if (h->stop_cb)
		h->stop_cb(h->cb_param);
}

static void _stream_moved_callback(pa_stream *s, void *userdata)
{
	assert(s);
	debug_msg("stream moved callback : %p", s);
}

static void _stream_underflow_callback(pa_stream *s, void *userdata)
{
	wave_info_t *h = (wave_info_t *)userdata;
	assert(s);

	debug_msg("stream underflow callback : %p, file(%s)", s, h->filename);
}

static void _stream_buffer_attr_callback(pa_stream *s, void *userdata)
{
	assert(s);
	debug_msg("stream underflow callback : %p", s);
}

static void _stream_write_callback(pa_stream *s, size_t length, void *userdata)
{
	sf_count_t bytes = 0;
	void *data = NULL;
	size_t frame_size;
	pa_operation *o = NULL;
	int ret = 0;
	wave_info_t *h = (wave_info_t *)userdata;

	if (!s || length <= 0) {
		debug_error("write error. stream(%p), length(%d)", s, length);
		return;
	}

	/* WAVE */
	data = pa_xmalloc(length);

	frame_size = pa_frame_size(&h->spec);

	if ((bytes = sf_readf_short(h->sf, data, (sf_count_t)(length / frame_size))) > 0)
		bytes *= (sf_count_t)frame_size;

	debug_msg("=== %lld / %d ===",bytes, length);

	if (bytes > 0)
		pa_stream_write(s, data, (size_t)bytes, pa_xfree, 0, PA_SEEK_RELATIVE);
	else
		pa_xfree(data);

	/* If No more data, drain stream */
	if (bytes < (sf_count_t)length) {
		debug_msg("EOS!!!!! %lld/%d", bytes, length);

		/* Handle loop */
		if (h->repeat_count == -1 || --h->repeat_count) {
			debug_msg("repeat count = %d", h->repeat_count);
			ret = sf_seek(h->sf, 0, SEEK_SET);
			if (ret == -1) {
				debug_error("SEEK failed .... ret = %d", ret);
				/* seek failed, can't loop anymore, do drain */
			} else {
				return;
			}
		}

		/* EOS callback will be notified after drain is completed */
		pa_stream_set_write_callback(s, NULL, NULL);
		o = pa_stream_drain(s, _stream_drain_complete, h);
		if (o)
			pa_operation_unref(o);
	}
}

static int _pa_connect_start_play(wave_info_t *h)
{
	pa_threaded_mainloop *m = NULL;
	pa_context *c = NULL;
	pa_stream *s = NULL;

	/* Mainloop */
	if (!(m = pa_threaded_mainloop_new())) {
		debug_error("mainloop create failed");
		goto error;
	}

	/* Context */
	if (!(c = pa_context_new(pa_threaded_mainloop_get_api(m), NULL))) {
		debug_msg("context create failed");
		goto error;
	}

	pa_context_set_state_callback(c, _context_state_callback, m);

	pa_threaded_mainloop_lock(m);

	if (pa_threaded_mainloop_start(m) < 0) {
		debug_error("mainloop start failed");
		goto error;
	}

	if (pa_context_connect(c, NULL, 0, NULL) < 0) {
		debug_error("context connect failed");
		goto error;
	}

	for (;;) {
		pa_context_state_t state = pa_context_get_state(c);
		if (state == PA_CONTEXT_READY)
			break;

		if (!PA_CONTEXT_IS_GOOD(state)) {
			debug_error("Context error!!!! %d", pa_context_errno(c));
			goto error;
		}

		pa_threaded_mainloop_wait(m);
	}

	/* Stream */
	pa_stream_flags_t flags = PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE;
	pa_proplist *proplist = pa_proplist_new();

	if (h->stream_type)
		pa_proplist_sets(proplist, PA_PROP_MEDIA_ROLE, h->stream_type);
	if (h->stream_index != -1)
		pa_proplist_setf(proplist, PA_PROP_MEDIA_PARENT_ID, "%d", h->stream_index);

	s = pa_stream_new_with_proplist(c, "wav-player", &h->spec, NULL, proplist);
	pa_proplist_free(proplist);
	if (!s) {
		debug_msg("pa_stream_new failed. file(%s)", h->filename);
		goto error;
	}

	pa_stream_set_state_callback(s, _stream_state_callback, m);
	pa_stream_set_write_callback(s, _stream_write_callback, h);
	pa_stream_set_moved_callback(s, _stream_moved_callback, h);
	pa_stream_set_underflow_callback(s, _stream_underflow_callback, h);
	pa_stream_set_buffer_attr_callback(s, _stream_buffer_attr_callback, h);

	pa_stream_connect_playback(s, NULL, NULL, flags, NULL, NULL);

	for (;;) {
		pa_stream_state_t state = pa_stream_get_state(s);

		if (state == PA_STREAM_READY)
			break;

		if (!PA_STREAM_IS_GOOD(state)) {
			debug_error("Stream error!!!! %d", pa_context_errno(c));
			goto error;
		}

		/* Wait until the stream is ready */
		pa_threaded_mainloop_wait(m);
	}

	h->m = m;
	h->c = c;
	h->s = s;

	pa_threaded_mainloop_unlock(m);

	return 0;

error:
	if (s)
		pa_stream_unref(s);

	if (c)
		pa_context_unref(c);

	if (m)
		pa_threaded_mainloop_free(m);

	return -1;
}

static int _pa_uncork(wave_info_t *h)
{
	pa_threaded_mainloop_lock(h->m);

	if (pa_stream_cork(h->s, 0, NULL, NULL) < 0) {
		debug_msg("pa_stream_cork(0)failed");
	}

	pa_threaded_mainloop_unlock(h->m);

	return 0;
}

static int _pa_stop_disconnect(wave_info_t *h)
{
	if (h->m) {
		pa_threaded_mainloop_lock(h->m);

		if (h->s) {
			pa_stream_disconnect(h->s);
			pa_stream_unref(h->s);
			h->s = NULL;
		}

		if (h->c) {
			pa_context_unref(h->c);
			h->c = NULL;
		}

		pa_threaded_mainloop_unlock(h->m);

		pa_threaded_mainloop_free(h->m);
		h->m = NULL;
	}
	return 0;
}

static int _prepare_sound(wave_info_t *h)
{
	memset(&h->si, 0, sizeof(SF_INFO));

	h->sf = sf_open(h->filename, SFM_READ, &h->si);
	if (!h->sf) {
		debug_msg("sf_open error. path(%s)", h->filename);
		return -1;
	}

	h->spec.rate = h->si.samplerate;
	h->spec.channels = h->si.channels;
	h->spec.format = PA_SAMPLE_S16LE;

	debug_msg("SF_INFO : frames = %lld, samplerate = %d, channels = %d, format = 0x%X, sections = %d, seekable = %d",
			h->si.frames, h->si.samplerate, h->si.channels, h->si.format, h->si.sections, h->si.seekable);

	return 0;
}

static void _unprepare_sound(wave_info_t *h)
{
	if (h->sf) {
		sf_close(h->sf);
		h->sf = NULL;
	}
}

static int (*g_thread_pool_func)(void*, void (*)(void*)) = NULL;

static int MMSoundPlugCodecWaveSetThreadPool(int (*func)(void*, void (*)(void*)))
{
	debug_enter("(func : %p)\n", func);
	g_thread_pool_func = func;
	debug_leave("\n");
	return MM_ERROR_NONE;
}

static int* MMSoundPlugCodecWaveGetSupportTypes(void)
{
	debug_enter("\n");
	static int suported[2] = {MM_SOUND_SUPPORTED_CODEC_WAVE, 0};
	debug_leave("\n");
	return suported;
}

static int MMSoundPlugCodecWaveParse(const char *filename, mmsound_codec_info_t *info)
{
	SNDFILE* sf;
	SF_INFO si;

	memset(&si, 0, sizeof(SF_INFO));
	sf = sf_open(filename, SFM_READ, &si);
	if (!sf) {
		debug_msg("sf_open [%s] error.....", filename);
		return MM_ERROR_SOUND_UNSUPPORTED_MEDIA_TYPE;
	}

	info->codec = MM_SOUND_SUPPORTED_CODEC_WAVE;
	info->channels = si.channels;
	info->samplerate = si.samplerate;

	debug_msg("filename = %s, frames[%lld], samplerate[%d], channels[%d], format[%x], sections[%d], seekable[%d]",
			filename, si.frames, si.samplerate, si.channels, si.format, si.sections, si.seekable);
	sf_close(sf);

	debug_leave("\n");
	return MM_ERROR_NONE;
}

static int MMSoundPlugCodecWaveCreate(mmsound_codec_param_t *param, mmsound_codec_info_t *info, MMHandleType *handle)
{
	wave_info_t* p = NULL;

#ifdef DEBUG_DETAIL
	debug_enter("\n");
#endif

	if (g_thread_pool_func == NULL) {
		debug_error("[CODEC WAV] Need thread pool!\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	p = (wave_info_t *) malloc(sizeof(wave_info_t));

	if (p == NULL) {
		debug_error("[CODEC WAV] memory allocation failed\n");
		return MM_ERROR_OUT_OF_MEMORY;
	}

	memset(p, 0, sizeof(wave_info_t));
	p->handle = 0;
	p->repeat_count = param ->repeat_count;
	p->stop_cb = param->stop_cb;
	p->cb_param = param->param;
	MMSOUND_STRNCPY(p->filename, param->pfilename, 256);
	p->stream_index = param->stream_index;
	MMSOUND_STRNCPY(p->stream_type, param->stream_type, MM_SOUND_STREAM_TYPE_LEN);

	_prepare_sound(p);
	_pa_connect_start_play(p);
	*handle = (MMHandleType)p;

#ifdef DEBUG_DETAIL
	debug_leave("%p\n", p);
#endif

	return MM_ERROR_NONE;
}


static int MMSoundPlugCodecWavePlay(MMHandleType handle)
{
	wave_info_t *p = (wave_info_t *) handle;

	debug_enter("(handle %x)\n", handle);

	_pa_uncork(p);

	debug_leave("\n");

	return MM_ERROR_NONE;
}

static int MMSoundPlugCodecWaveStop(MMHandleType handle)
{
	int ret = 0;
	wave_info_t *p = (wave_info_t*) handle;

	if (!p) {
		debug_error("The handle is null\n");
		return MM_ERROR_SOUND_INTERNAL;
	}

	debug_msg("[CODEC WAV] Handle 0x%08X stop requested\n", handle);

	ret = _pa_stop_disconnect(p);

	return MM_ERROR_NONE;
}

static int MMSoundPlugCodecWaveDestroy(MMHandleType handle)
{
	wave_info_t *p = (wave_info_t *)handle;

	if (!p) {
		debug_warning("Can not destroy handle :: handle is invalid");
		return MM_ERROR_SOUND_INVALID_POINTER;
	}

	_unprepare_sound(p);

	free(p);
	p = NULL;

	return MM_ERROR_NONE;
}

EXPORT_API
int MMSoundPlugCodecGetInterface(mmsound_codec_interface_t *intf)
{
	intf->GetSupportTypes   = MMSoundPlugCodecWaveGetSupportTypes;
	intf->Parse             = MMSoundPlugCodecWaveParse;
	intf->Create            = MMSoundPlugCodecWaveCreate;
	intf->Destroy           = MMSoundPlugCodecWaveDestroy;
	intf->Play              = MMSoundPlugCodecWavePlay;
	intf->Stop              = MMSoundPlugCodecWaveStop;
	intf->SetThreadPool     = MMSoundPlugCodecWaveSetThreadPool;

	return MM_ERROR_NONE;
}

EXPORT_API
int MMSoundGetPluginType(void)
{
	return MM_SOUND_PLUGIN_TYPE_CODEC;
}


