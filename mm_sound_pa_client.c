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

#include <mm_error.h>
#include <mm_debug.h>

#include <mm_sound_pa_client.h>

#include <glib.h>

#define MM_SOUND_CHANNEL_MIN                1
#define MM_SOUND_CHANNEL_MAX                6

typedef struct _mm_sound_handle_t {
    uint32_t handle;

    int mode;
    int policy;
    int volume_type;
    int gain_type;
    int rate;
    int channels;
    pa_simple* s;

    int period; /* open api retrun value.*/

    unsigned int stream_idx;
    int source_type;
} mm_sound_handle_t;

#define MM_SOUND_HANDLE_MAX                 32
static struct {
    uint32_t handle_count; /* use amotic operations */
    GList* handles;
    pthread_mutex_t lock;

    int state;
    pa_threaded_mainloop *mainloop;
    pa_context *context;
} mm_sound_handle_mgr;

#define CHECK_HANDLE_RANGE(x) \
    do { \
        if(x == 0) { \
            debug_msg("invalid handle(%d)", x); \
            return MM_ERROR_INVALID_ARGUMENT; \
        } \
    } while(0);

#define CHECK_VOLUME_TYPE_RANGE(x) \
    do { \
        if(x < VOLUME_TYPE_SYSTEM || x >= VOLUME_TYPE_MAX) { \
            debug_msg("invalid volume type(%d)", x); \
            return MM_ERROR_INVALID_ARGUMENT; \
        } \
    } while(0);

#define ATOMIC_INC(l, x) \
    do { \
        pthread_mutex_lock(l); \
        x = x + 1; \
        if(x == 0) \
            x = x + 1; \
        pthread_mutex_unlock(l); \
    } while(0);

// phandle(ret), GList, userdata, coimpare func
#define GET_HANDLE_DATA(p, l, u, func) \
    do { \
        GList* list = 0; \
        list = g_list_find_custom(l, u, func); \
        if(list != 0) \
            p = (mm_sound_handle_t*)list->data; \
        else \
            p = NULL; \
    } while(0);


// should be call after pa_ext function.
#define WAIT_PULSEAUDIO_OPERATION(x, y) \
    do { \
        while (pa_operation_get_state(y) == PA_OPERATION_RUNNING) { \
            debug_msg("waiting.................."); \
            pa_threaded_mainloop_wait(x.mainloop); \
            debug_msg("waiting DONE"); \
        } \
    } while(0);

#define PA_SIMPLE_FADE_INTERVAL_USEC						20000

#define PA_SIMPLE_SAMPLES_PER_PERIOD_DEFAULT				1536	/* frames */
#define PA_SIMPLE_PERIODS_PER_BUFFER_FASTMODE				4
#define PA_SIMPLE_PERIODS_PER_BUFFER_DEFAULT				6
#define PA_SIMPLE_PERIODS_PER_BUFFER_VOIP					2
#define PA_SIMPLE_PERIODS_PER_BUFFER_PLAYBACK				8
#define PA_SIMPLE_PERIODS_PER_BUFFER_CAPTURE				12
#define PA_SIMPLE_PERIODS_PER_BUFFER_VIDEO					10

#define PA_SIMPLE_PERIOD_TIME_FOR_ULOW_LATENCY_MSEC			20
#define PA_SIMPLE_PERIOD_TIME_FOR_LOW_LATENCY_MSEC			25
#define PA_SIMPLE_PERIOD_TIME_FOR_MID_LATENCY_MSEC			50
#define PA_SIMPLE_PERIOD_TIME_FOR_HIGH_LATENCY_MSEC			75
#define PA_SIMPLE_PERIOD_TIME_FOR_VERY_HIGH_LATENCY_MSEC		150
#define PA_SIMPLE_PERIOD_TIME_FOR_VOIP_LATENCY_MSEC			20

#define IS_INPUT_HANDLE(x) \
    if( x == HANDLE_MODE_INPUT || x == HANDLE_MODE_INPUT_HIGH_LATENCY || \
        x == HANDLE_MODE_INPUT_LOW_LATENCY || x == HANDLE_MODE_INPUT_AP_CALL )

__attribute__ ((constructor)) void __mm_sound_pa_init(void)
{
    memset(&mm_sound_handle_mgr, 0, sizeof(mm_sound_handle_mgr));
    mm_sound_handle_mgr.state = FALSE;

    mm_sound_handle_mgr.handles = g_list_alloc();
    mm_sound_handle_mgr.handle_count = 1;
    pthread_mutex_init(&mm_sound_handle_mgr.lock, NULL);
}

__attribute__ ((destructor)) void __mm_sound_pa_deinit(void)
{
     g_list_free(mm_sound_handle_mgr.handles);
     pthread_mutex_destroy(&mm_sound_handle_mgr.lock);
     mm_sound_handle_mgr.handle_count = 0;

    if(mm_sound_handle_mgr.state) {
        debug_msg("mainloop(%x),  context(%x)", mm_sound_handle_mgr.mainloop, mm_sound_handle_mgr.context);
        pa_threaded_mainloop_stop(mm_sound_handle_mgr.mainloop);
        pa_context_disconnect(mm_sound_handle_mgr.context);
        pa_context_unref(mm_sound_handle_mgr.context);
        pa_threaded_mainloop_free(mm_sound_handle_mgr.mainloop);
    }
}

gint __mm_sound_handle_comparefunc(gconstpointer a, gconstpointer b)
{
    mm_sound_handle_t* phandle = (mm_sound_handle_t*)a;
    int* handle = (int*)b;

    if(phandle == NULL)
        return -1;

    if(phandle->handle == *handle)
        return 0;
    else
        return -1;
}

EXPORT_API
int mm_sound_pa_open(MMSoundHandleMode mode, mm_sound_handle_route_info *route_info, MMSoundHandlePriority priority, int volume_config, pa_sample_spec* ss, pa_channel_map* channel_map, int* size, char *stream_type, int stream_index)
{
    pa_simple *s = NULL;
    pa_channel_map maps;
    pa_buffer_attr attr;

    int prop_vol_type = 0;
    int prop_gain_type = VOLUME_GAIN_DEFAULT;

    int err = MM_ERROR_SOUND_INTERNAL;
    int period_time = PA_SIMPLE_PERIOD_TIME_FOR_MID_LATENCY_MSEC;
    int samples_per_period = PA_SIMPLE_SAMPLES_PER_PERIOD_DEFAULT;
    int periods_per_buffer = PA_SIMPLE_PERIODS_PER_BUFFER_DEFAULT;

    int handle_mode = mode;
    int handle_inout = HANDLE_DIRECTION_NONE;
    int sample_size = 0;
    pa_proplist *proplist = NULL;

    mm_sound_handle_t* handle = NULL;
    MMSoundHandleRoutePolicy policy = HANDLE_ROUTE_POLICY_DEFAULT;

    if(route_info) {
        policy = route_info->policy;
    }

    if (ss->channels < MM_SOUND_CHANNEL_MIN || ss->channels > MM_SOUND_CHANNEL_MAX)
        return MM_ERROR_INVALID_ARGUMENT;

    proplist = pa_proplist_new();

    if(channel_map == NULL) {
        pa_channel_map_init_auto(&maps, ss->channels, PA_CHANNEL_MAP_ALSA);
        channel_map = &maps;
    }

    switch(ss->format) {
        case PA_SAMPLE_U8:
            sample_size = 1 * ss->channels;
            break;
        case PA_SAMPLE_S16LE:
            sample_size = 2 * ss->channels;
            break;
        default :
            sample_size = 0;
            debug_error("Invalid sample size (%d)", sample_size);
            break;
    }

    /* Set volume type of stream */
    if(volume_config > 0) {
        debug_log("setting gain type");
        prop_vol_type = 0; /* not used, set it system(0) temporarily */

        /* Set gain type of stream */
        prop_gain_type = (volume_config >> 8) & 0x000000FF;

        pa_proplist_setf(proplist, PA_PROP_MEDIA_TIZEN_GAIN_TYPE, "%d", prop_gain_type);
    }

    if (stream_index != -1) {
        char stream_index_s[11];
        debug_msg("Set stream index [%d]", stream_index);

        snprintf(stream_index_s, sizeof(stream_index_s)-1, "%d", stream_index);
        debug_msg("stream_index[%d] converted to string[%s]", stream_index, stream_index_s);
        pa_proplist_sets(proplist, PA_PROP_MEDIA_PARENT_ID, stream_index_s);
    }
    /* Set stream type */
    pa_proplist_sets(proplist, PA_PROP_MEDIA_ROLE, stream_type);

    memset(&attr, '\0', sizeof(attr));

    switch (handle_mode) {
    case HANDLE_MODE_INPUT:
        period_time = PA_SIMPLE_PERIOD_TIME_FOR_MID_LATENCY_MSEC;
        samples_per_period = (ss->rate * period_time) / 1000;
        periods_per_buffer = PA_SIMPLE_PERIODS_PER_BUFFER_DEFAULT;
        attr.prebuf = 0;
        attr.minreq = -1;
        attr.tlength = -1;
        attr.maxlength = -1;
        attr.fragsize = samples_per_period * pa_sample_size(ss);

        s = pa_simple_new_proplist(NULL, "MM_SOUND_PA_CLIENT", PA_STREAM_RECORD, NULL, "CAPTURE", ss, channel_map, &attr, proplist, &err);
        break;

    case HANDLE_MODE_INPUT_LOW_LATENCY:
        period_time = PA_SIMPLE_PERIOD_TIME_FOR_ULOW_LATENCY_MSEC;
        samples_per_period = (ss->rate * period_time) / 1000;
        periods_per_buffer = PA_SIMPLE_PERIODS_PER_BUFFER_FASTMODE;
        attr.prebuf = 0;
        attr.minreq = -1;
        attr.tlength = -1;
        attr.maxlength = -1;
        attr.fragsize = samples_per_period * pa_sample_size(ss);

        s = pa_simple_new_proplist(NULL, "MM_SOUND_PA_CLIENT", PA_STREAM_RECORD, NULL, "LOW LATENCY CAPTURE", ss, channel_map, &attr, proplist, &err);
        break;

    case HANDLE_MODE_INPUT_HIGH_LATENCY:
        period_time = PA_SIMPLE_PERIOD_TIME_FOR_HIGH_LATENCY_MSEC;
        samples_per_period = (ss->rate * period_time) / 1000;
        periods_per_buffer = PA_SIMPLE_PERIODS_PER_BUFFER_CAPTURE;
        attr.prebuf = 0;
        attr.minreq = -1;
        attr.tlength = -1;
        attr.maxlength = -1;
        attr.fragsize = samples_per_period * pa_sample_size(ss);

        s = pa_simple_new_proplist(NULL, "MM_SOUND_PA_CLIENT", PA_STREAM_RECORD, NULL, "HIGH LATENCY CAPTURE", ss, channel_map, &attr, proplist, &err);
        break;

    case HANDLE_MODE_OUTPUT:
        period_time = PA_SIMPLE_PERIOD_TIME_FOR_MID_LATENCY_MSEC;
        samples_per_period = (ss->rate * period_time) / 1000;
        periods_per_buffer = PA_SIMPLE_PERIODS_PER_BUFFER_DEFAULT;
        attr.prebuf = -1;
        attr.minreq = -1;
        attr.tlength = (ss->rate / 10) * pa_sample_size(ss) * ss->channels;
        attr.maxlength = -1;
        attr.fragsize = 0;

        s = pa_simple_new_proplist(NULL, "MM_SOUND_PA_CLIENT", PA_STREAM_PLAYBACK, NULL, "PLAYBACK", ss, channel_map, &attr, proplist, &err);
        break;

    case HANDLE_MODE_OUTPUT_LOW_LATENCY:
        period_time = PA_SIMPLE_PERIOD_TIME_FOR_LOW_LATENCY_MSEC;
        samples_per_period = (ss->rate * period_time) / 1000;
        periods_per_buffer = PA_SIMPLE_PERIODS_PER_BUFFER_FASTMODE;
        attr.prebuf = (ss->rate / 100) * pa_sample_size(ss) * ss->channels;
        attr.minreq = -1;
        attr.tlength = (ss->rate / 10) * pa_sample_size(ss) * ss->channels;
        attr.maxlength = -1;
        attr.fragsize = 0;
        debug_msg("rate(%d), samplesize(%d), ch(%d) format(%d)", ss->rate, pa_sample_size(ss), ss->channels, ss->format);

        debug_msg("prebuf(%d), minreq(%d), tlength(%d), maxlength(%d), fragsize(%d)", attr.prebuf, attr.minreq, attr.tlength, attr.maxlength, attr.fragsize);

        s = pa_simple_new_proplist(NULL,"MM_SOUND_PA_CLIENT", PA_STREAM_PLAYBACK, NULL, "LOW LATENCY PLAYBACK", ss, channel_map, &attr, proplist, &err);
        break;

    case HANDLE_MODE_OUTPUT_CLOCK:
        period_time = PA_SIMPLE_PERIOD_TIME_FOR_HIGH_LATENCY_MSEC;
        samples_per_period = (ss->rate * period_time) / 1000;
        periods_per_buffer = PA_SIMPLE_PERIODS_PER_BUFFER_PLAYBACK;
        attr.prebuf = -1;
        attr.minreq = -1;
        attr.tlength = (uint32_t)-1;
        attr.maxlength = -1;
        attr.fragsize = 0;

        s = pa_simple_new_proplist(NULL, "MM_SOUND_PA_CLIENT", PA_STREAM_PLAYBACK, NULL, "HIGH LATENCY PLAYBACK", ss, channel_map, &attr, proplist, &err);
        break;

    case HANDLE_MODE_OUTPUT_VIDEO: /* low latency playback */
        period_time = PA_SIMPLE_PERIOD_TIME_FOR_LOW_LATENCY_MSEC;
        samples_per_period = (ss->rate * period_time) / 1000;
        periods_per_buffer = PA_SIMPLE_PERIODS_PER_BUFFER_VIDEO;
        attr.prebuf = 4*(samples_per_period * pa_sample_size(ss));
        attr.minreq = samples_per_period * pa_sample_size(ss);
        attr.tlength = periods_per_buffer * samples_per_period * pa_sample_size(ss);
        attr.maxlength = -1;
        attr.fragsize = 0;

        s = pa_simple_new_proplist(NULL, "MM_SOUND_PA_CLIENT", PA_STREAM_PLAYBACK, NULL, "LOW LATENCY PLAYBACK", ss, channel_map, &attr, proplist, &err);
        break;

    case HANDLE_MODE_OUTPUT_AP_CALL:
#if defined(_MMFW_I386_ALL_SIMULATOR)
        debug_msg("Does not support AP call mode at i386 simulator\n");
        s = NULL;
#else
        period_time = PA_SIMPLE_PERIOD_TIME_FOR_VOIP_LATENCY_MSEC;
        samples_per_period = (ss->rate * period_time) / 1000;
        periods_per_buffer = PA_SIMPLE_PERIODS_PER_BUFFER_VOIP;
        attr.prebuf = -1;
        attr.minreq = pa_usec_to_bytes(20*PA_USEC_PER_MSEC, ss);
        attr.tlength = pa_usec_to_bytes(100*PA_USEC_PER_MSEC, ss);
        attr.maxlength = -1;
        attr.fragsize = 0;

        s = pa_simple_new_proplist(NULL, "MM_SOUND_PA_CLIENT", PA_STREAM_PLAYBACK, NULL, "VoIP PLAYBACK", ss, channel_map, &attr, proplist, &err);
#endif
        break;
    case HANDLE_MODE_INPUT_AP_CALL:
#if defined(_MMFW_I386_ALL_SIMULATOR)
        debug_msg("Does not support AP call mode at i386 simulator\n");
        s = NULL;
#else
        period_time = PA_SIMPLE_PERIOD_TIME_FOR_VOIP_LATENCY_MSEC;
        samples_per_period = (ss->rate * period_time) / 1000;
        periods_per_buffer = PA_SIMPLE_PERIODS_PER_BUFFER_VOIP;
        attr.prebuf = 0;
        attr.minreq = -1;
        attr.tlength = -1;
        attr.maxlength = -1;
        attr.fragsize = samples_per_period * pa_sample_size(ss);

        s = pa_simple_new_proplist(NULL, "MM_SOUND_PA_CLIENT", PA_STREAM_RECORD, NULL, "VoIP CAPTURE", ss, channel_map, &attr, proplist, &err);
#endif
        break;
    default:
        err = MM_ERROR_SOUND_INTERNAL;
        goto fail;
        break;
    }

    if (!s) {
        debug_error("Open pulseaudio handle has failed - %s\n", pa_strerror(err));
        if (!strncmp(pa_strerror(err), "Access denied by security check",strlen(pa_strerror(err)))) {
            err = MM_ERROR_SOUND_PERMISSION_DENIED;
        } else {
            err = MM_ERROR_SOUND_INTERNAL;
        }
        goto fail;
    }

    handle = (mm_sound_handle_t*)malloc(sizeof(mm_sound_handle_t));
    memset(handle, 0, sizeof(mm_sound_handle_t));
    handle->mode = mode;
    handle->policy = policy;
    handle->volume_type = prop_vol_type;
    handle->gain_type = prop_gain_type;
    handle->rate = ss->rate;
    handle->channels = ss->channels;
    handle->s = s;
    handle->period = samples_per_period * sample_size;
    *size = handle->period;
    handle->handle = mm_sound_handle_mgr.handle_count;
    ATOMIC_INC(&mm_sound_handle_mgr.lock, mm_sound_handle_mgr.handle_count); // 0 is not used

    if (0 > pa_simple_get_stream_index(s, &handle->stream_idx, &err)) {
        debug_msg("Can not get stream index %s\n", pa_strerror(err));
        err = MM_ERROR_SOUND_INTERNAL;
        goto fail;
    }
    mm_sound_handle_mgr.handles = g_list_append(mm_sound_handle_mgr.handles, handle);

    if(handle->handle == 0) {
        debug_msg("out of range. handle(%d)\n", handle->handle);
        goto fail;
    }

    debug_msg("created handle[%d]. mode(%d), policy(%d), volumetype(%d), gain(%d), rate(%d), channels(%d), format(%d), stream_idx(%d), s(%x), source_type(%d) handle_inout(%d)",
        handle->handle, handle->mode, handle->policy, handle->volume_type, handle->gain_type,
        handle->rate, handle->channels, ss->format, handle->stream_idx, handle->s, handle->source_type, handle_inout);

    return handle->handle;

fail:
    if (proplist)
        pa_proplist_free(proplist);

    if(handle)
        free(handle);

    return err;

}

EXPORT_API
int mm_sound_pa_read(const int handle, void* buf, const int size)
{
    mm_sound_handle_t* phandle = NULL;
    int err = MM_ERROR_NONE;

#ifdef __STREAM_DEBUG__
    debug_msg("handle(%d), buf(%p), size(%d)", handle, buf, size);
#endif
    if (buf == NULL)
        return MM_ERROR_INVALID_ARGUMENT;

    if (size < 0)
        return MM_ERROR_INVALID_ARGUMENT;
    else if (size == 0)
        return size;

    CHECK_HANDLE_RANGE(handle);
    GET_HANDLE_DATA(phandle, mm_sound_handle_mgr.handles, &handle, __mm_sound_handle_comparefunc);
    if(phandle == NULL) {
        debug_msg("phandle is null");
        return MM_ERROR_SOUND_INTERNAL;
    }

    if (0 > pa_simple_read(phandle->s, buf, size, &err)) {
        debug_error("pa_simple_read() failed with %s", pa_strerror(err));
        return MM_ERROR_SOUND_INTERNAL;
    }

    return size;
}

EXPORT_API
int mm_sound_pa_write(const int handle, void* buf, const int size)
{
    mm_sound_handle_t* phandle = NULL;
    int err = MM_ERROR_NONE;

    if (buf == NULL)
        return MM_ERROR_INVALID_ARGUMENT;

    if (size < 0)
        return MM_ERROR_INVALID_ARGUMENT;
    else if (size == 0)
        return MM_ERROR_NONE;

    CHECK_HANDLE_RANGE(handle);
    GET_HANDLE_DATA(phandle, mm_sound_handle_mgr.handles, &handle, __mm_sound_handle_comparefunc);

#ifdef __STREAM_DEBUG__
    debug_msg("phandle(%x) s(%x), handle(%d), rate(%d), ch(%d) stream_idx(%d), buf(%p), size(%d)",
        phandle, phandle->s, phandle->handle, phandle->rate, phandle->channels, phandle->stream_idx. buf, size);
#endif

    if(phandle == NULL) {
        debug_msg("phandle is null");
        return MM_ERROR_SOUND_INTERNAL;
    }

    if (0 > pa_simple_write(phandle->s, buf, size, &err)) {
        debug_error("pa_simple_write() failed with %s\n", pa_strerror(err));
        return MM_ERROR_SOUND_INTERNAL;
    }

    return size;

}

EXPORT_API
int mm_sound_pa_close(const int handle)
{
    mm_sound_handle_t* phandle = NULL;
    int err = MM_ERROR_NONE;

    CHECK_HANDLE_RANGE(handle);
    GET_HANDLE_DATA(phandle, mm_sound_handle_mgr.handles, &handle, __mm_sound_handle_comparefunc);
    if(phandle == NULL) {
        debug_msg("phandle is null");
        return MM_ERROR_SOUND_INTERNAL;
    }

    debug_msg("phandle(%x) s(%x), handle(%d), rate(%d), ch(%d) stream_idx(%d)",
            phandle, phandle->s, phandle->handle, phandle->rate, phandle->channels, phandle->stream_idx);

	switch (phandle->mode) {
	case HANDLE_MODE_OUTPUT:
	case HANDLE_MODE_OUTPUT_CLOCK:
	case HANDLE_MODE_OUTPUT_LOW_LATENCY:
	case HANDLE_MODE_OUTPUT_AP_CALL:
	case HANDLE_MODE_OUTPUT_VIDEO:
		if (0 > pa_simple_flush(phandle->s, &err)) {
            err = MM_ERROR_SOUND_INTERNAL;
			debug_msg("pa_simple_flush() failed with %s\n", pa_strerror(err));
		}
		break;
	default:
		break;
	}

	pa_simple_free(phandle->s);
    phandle->s = NULL;

    debug_msg("leave: handle[%d] stream_index[%d]\n", handle, phandle->stream_idx);

    mm_sound_handle_mgr.handles = g_list_remove(mm_sound_handle_mgr.handles, phandle);
    if(phandle != NULL) {
        free(phandle);
        phandle = NULL;
    }

    return err;

}

EXPORT_API
int mm_sound_pa_drain(const int handle)
{
    mm_sound_handle_t* phandle = NULL;
    int err = MM_ERROR_NONE;

	CHECK_HANDLE_RANGE(handle);
    GET_HANDLE_DATA(phandle, mm_sound_handle_mgr.handles, &handle, __mm_sound_handle_comparefunc);
    if(phandle == NULL)
        return MM_ERROR_SOUND_INTERNAL;

    if (0 > pa_simple_drain(phandle->s, &err)) {
		debug_error("pa_simple_drain() failed with %s\n", pa_strerror(err));
		err = MM_ERROR_SOUND_INTERNAL;
	}

	return err;
}

EXPORT_API
int mm_sound_pa_flush(const int handle)
{
    mm_sound_handle_t* phandle = NULL;
    int err = MM_ERROR_NONE;

    CHECK_HANDLE_RANGE(handle);
    GET_HANDLE_DATA(phandle, mm_sound_handle_mgr.handles, &handle, __mm_sound_handle_comparefunc);
    if(phandle == NULL)
        return MM_ERROR_SOUND_INTERNAL;

    if (0 > pa_simple_flush(phandle->s, &err)) {
        debug_error("pa_simple_flush() failed with %s\n", pa_strerror(err));
        err = MM_ERROR_SOUND_INTERNAL;
    }

    return err;
}

typedef struct _get_volume_max_userdata_t
{
    pa_threaded_mainloop* mainloop;
    int value;
} get_volume_max_userdata_t;
