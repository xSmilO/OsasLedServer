#include "spa/param/param.h"
#include <gio/gio.h>
#include <glib.h>
#include <iostream>
#include <libportal/portal.h>
#include <libportal/remote.h>
#include <libportal/session.h>

#include <pipewire/pipewire.h>
#include <pipewire/thread-loop.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/video/raw.h>

#define TOP_LEDS 50
#define SIDE_LEDS 20

struct AppData {
    GMainLoop *loop;
    XdpPortal *portal;

    struct pw_thread_loop *thread_loop;
    struct pw_context *pw_context;
    struct pw_core *pw_core;
    struct pw_stream *pw_stream;
    struct spa_hook stream_listener;

    int width;
    int height;
    int stride;
    int bpp;

    int red_offset;
    int blue_offset;
    int green_offset;
};

struct PIXEL_DATA {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

static PIXEL_DATA getColorFromArea(const uint16_t &x1, const uint16_t &x2,
                                   const uint16_t &y1, const uint16_t &y2,
                                   const uint8_t *pixels,
                                   const AppData *userData) {
    PIXEL_DATA result = {0, 0, 0};

    uint16_t pixelCount = (x2 - x1) * (y2 - y1);
    uint32_t redSum = 0;
    uint32_t greenSum = 0;
    uint32_t blueSum = 0;
    for (uint16_t y = y1; y < y2; ++y) {
        for (uint16_t x = x1; x < x2; ++x) {
            int offset = (y * userData->stride + (x * userData->bpp));
            redSum += pixels[offset + userData->red_offset];
            greenSum += pixels[offset + userData->green_offset];
            blueSum += pixels[offset + userData->blue_offset];
        }
    }

    result.r = redSum / pixelCount;
    result.g = greenSum / pixelCount;
    result.b = blueSum / pixelCount;

    return result;
}

static void on_process(void *userdata) {
    AppData *data = (AppData *)userdata;
    struct pw_buffer *b;
    struct spa_buffer *buf;

    if ((b = pw_stream_dequeue_buffer(data->pw_stream)) == NULL) {
        return; // no ready buffer;
    }

    buf = b->buffer;

    unsigned int x = data->width / 2, y = data->height / 2;
    uint16_t width_split = data->width / TOP_LEDS;
    uint16_t height_split = data->height / SIDE_LEDS;
    uint8_t padding = 60;

    if (buf->datas[0].data != NULL) {
        uint8_t *p = (uint8_t *)buf->datas[0].data;

        // top screen
        PIXEL_DATA pd = getColorFromArea(0, padding, 0, padding, p, data);
        std::cout<<"___\n";
        std::cout<<"R: " << (int)pd.r << " G: " << (int)pd.g << " B: " << (int)pd.b << std::endl;
        /*
        if (x < data->width && y < data->height) {
            int offset = (y * data->stride + (x * data->bpp));

            if (offset + data->bpp <= buf->datas[0].chunk->size) {
                uint8_t r, g, b;

                r = p[offset + data->red_offset];
                g = p[offset + data->green_offset];
                b = p[offset + data->blue_offset];

                std::cout << "[Pixel " << x << "," << y << "]" << "[" << (int)r
                          << ", " << (int)g << ", " << (int)b << "]\n";
            }
        }
        */
    }

    pw_stream_queue_buffer(data->pw_stream, b);
}

static void on_state_changed(void *userdata, enum pw_stream_state old,
                             enum pw_stream_state state, const char *error) {
    std::cout << "[PW THREAD] Stream State: "
              << pw_stream_state_as_string(state) << std::endl;
    if (state == PW_STREAM_STATE_ERROR) {
        std::cerr << "Stream Error: " << (error ? error : "Uknown")
                  << std::endl;
    }
}

static void on_param_changed(void *userdata, uint32_t id,
                             const struct spa_pod *param) {
    AppData *data = (AppData *)userdata;

    if (param == NULL || id != SPA_PARAM_Format)
        return;

    struct spa_video_info_raw info;
    if (spa_format_video_raw_parse(param, &info) < 0)
        return;

    data->width = info.size.width;
    data->height = info.size.height;

    switch (info.format) {
    case SPA_VIDEO_FORMAT_BGRA:
    case SPA_VIDEO_FORMAT_BGRx:
        data->bpp = 4;
        data->red_offset = 2;
        data->green_offset = 1;
        data->blue_offset = 0;
        break;
    case SPA_VIDEO_FORMAT_RGBA:
    case SPA_VIDEO_FORMAT_RGBx:
        data->bpp = 4;
        data->red_offset = 0;
        data->green_offset = 1;
        data->blue_offset = 2;
        break;
    case SPA_VIDEO_FORMAT_RGB:
        data->bpp = 3;
        data->red_offset = 0;
        data->green_offset = 1;
        data->blue_offset = 2;
        break;
    default:
        std::cout << "ERROR: UNHANDLED FORMAT\n";
        break;
    }

    data->stride = data->width * data->bpp;

    std::cout << "Format negotiated: " << data->width << "x" << data->height
              << " BPP:" << data->bpp << " Stride:" << data->stride
              << std::endl;

    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    const struct spa_pod *params[1];
    params[0] = (const struct spa_pod *)spa_pod_builder_add_object(
        &b, SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
        SPA_PARAM_BUFFERS_buffers, SPA_POD_Int(4), SPA_PARAM_BUFFERS_blocks,
        SPA_POD_Int(1), SPA_PARAM_BUFFERS_size,
        SPA_POD_Int(data->stride * data->height), SPA_PARAM_BUFFERS_stride,
        SPA_POD_Int(data->stride));

    pw_stream_update_params(data->pw_stream, params, 1);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .state_changed = on_state_changed,
    .param_changed = on_param_changed,
    .process = on_process,
};

static void start_pipewire(const uint32_t &node_id, AppData *data) {
    std::cout << "PipeWire: Starting thread loop..." << std::endl;
    data->thread_loop = pw_thread_loop_new("PixelReaderLoop", NULL);

    data->pw_context =
        pw_context_new(pw_thread_loop_get_loop(data->thread_loop), NULL, 0);

    pw_thread_loop_lock(data->thread_loop);

    data->pw_core = pw_context_connect(data->pw_context, NULL, 0);

    if (!data->pw_core) {
        std::cerr << "PipeWire: Failed to connect to core." << std::endl;
        return;
    }

    data->pw_stream = pw_stream_new(
        data->pw_core, "Pixel Reader",
        pw_properties_new(PW_KEY_MEDIA_TYPE, "Video", PW_KEY_MEDIA_CATEGORY,
                          "Capture", NULL));

    pw_stream_add_listener(data->pw_stream, &data->stream_listener,
                           &stream_events, data);

    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    const struct spa_pod *params[5];

    struct spa_video_info_raw info_bgra = {};
    info_bgra.format = SPA_VIDEO_FORMAT_BGRA;
    struct spa_video_info_raw info_rgba = {};
    info_rgba.format = SPA_VIDEO_FORMAT_RGBA;
    struct spa_video_info_raw info_bgrx = {};
    info_bgrx.format = SPA_VIDEO_FORMAT_BGRx;
    struct spa_video_info_raw info_rgbx = {};
    info_rgbx.format = SPA_VIDEO_FORMAT_RGBx;
    struct spa_video_info_raw info_rgb = {};
    info_rgb.format = SPA_VIDEO_FORMAT_RGB;

    params[0] = spa_format_video_raw_build(&b, SPA_PARAM_EnumFormat,
                                           &info_bgra); // Najczęstszy
    params[1] =
        spa_format_video_raw_build(&b, SPA_PARAM_EnumFormat, &info_rgba);
    params[2] =
        spa_format_video_raw_build(&b, SPA_PARAM_EnumFormat, &info_bgrx);
    params[3] =
        spa_format_video_raw_build(&b, SPA_PARAM_EnumFormat, &info_rgbx);
    params[4] = spa_format_video_raw_build(&b, SPA_PARAM_EnumFormat, &info_rgb);
    pw_stream_connect(data->pw_stream, PW_DIRECTION_INPUT, node_id,
                      (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                                        PW_STREAM_FLAG_MAP_BUFFERS),
                      params, 3);

    pw_thread_loop_unlock(data->thread_loop);

    std::cout << "PipeWire: Starting processing thread..." << std::endl;
    pw_thread_loop_start(data->thread_loop);
}

static void on_screencast_start(GObject *source_object, GAsyncResult *res,
                                gpointer user_data) {
    AppData *data = (AppData *)user_data;

    XdpSession *session = (XdpSession *)source_object;
    GError *error = NULL;

    if (xdp_session_start_finish(session, res, &error)) {
        std::cout << "Session started by user! Looking for ID.." << std::endl;

        GVariant *streams = xdp_session_get_streams(session);

        if (streams) {
            GVariantIter iter;
            g_variant_iter_init(&iter, streams);
            uint32_t node_id;
            GVariant *options = NULL;

            if (g_variant_iter_next(&iter, "(u@a{sv})", &node_id, &options)) {
                std::cout << "Portal: Success! Node ID: " << node_id
                          << std::endl;

                start_pipewire(node_id, data);
            }

            g_variant_unref(streams);
        }
    } else {
        // cleanup
        if (error) {
            std::cerr << "Start sesssion error: " << error->message
                      << std::endl;
            g_error_free(error);
        }
        g_main_loop_quit(data->loop);
    }
}
static void on_screencast_created(GObject *source_object, GAsyncResult *res,
                                  gpointer user_data) {
    AppData *data = (AppData *)user_data;
    XdpPortal *portal = XDP_PORTAL(source_object);
    GError *error = NULL;

    XdpSession *session =
        xdp_portal_create_screencast_session_finish(portal, res, &error);

    if (error) {
        std::cerr << "Error in session creation: " << error->message
                  << std::endl;
        g_error_free(error);
        g_main_loop_quit(data->loop);

        return;
    }

    std::cout << "Session successfully created!" << std::endl;

    xdp_session_start(session, NULL, NULL, on_screencast_start, user_data);
}

/*

int main(int argc, char **argv) {
    pw_init(&argc, &argv);

    AppData data = {0};
    data.loop = g_main_loop_new(NULL, FALSE);
    data.portal = xdp_portal_new();

    xdp_portal_create_screencast_session(
        data.portal, XDP_OUTPUT_MONITOR, XDP_SCREENCAST_FLAG_NONE,
        XDP_CURSOR_MODE_EMBEDDED, XDP_PERSIST_MODE_PERSISTENT, NULL, NULL,
        on_screencast_created, &data);

    g_main_loop_run(data.loop);

    return 0;
}

*/
