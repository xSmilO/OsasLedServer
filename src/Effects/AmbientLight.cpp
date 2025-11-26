#include <Effects/AmbientLight.h>
#include <iostream>

uint8_t AmbientLight::LEFT_LEDS = 0;
uint8_t AmbientLight::RIGHT_LEDS = 0;
uint8_t AmbientLight::TOP_LEDS = 0;
uint16_t AmbientLight::PADDING = 0;

void AmbientLight::on_state_changed(void *userdata, enum pw_stream_state old,
                                    enum pw_stream_state state,
                                    const char *error) {
    std::cout << "[PW THREAD] Stream State: "
              << pw_stream_state_as_string(state) << std::endl;
    if (state == PW_STREAM_STATE_ERROR) {
        std::cerr << "Stream Error: " << (error ? error : "Uknown")
                  << std::endl;
    }
}

void AmbientLight::on_param_changed(void *userdata, uint32_t id,
                                    const struct spa_pod *param) {
    AmbientLightData *data = (AmbientLightData *)userdata;

    if (param == NULL || id != SPA_PARAM_Format)
        return;

    struct spa_video_info_raw info;
    if (spa_format_video_raw_parse(param, &info) < 0)
        return;

    data->width = info.size.width;
    data->height = info.size.height;

    data->topPixelsBlock = data->width / AmbientLight::TOP_LEDS;
    data->leftPixelsBlock = data->height / AmbientLight::LEFT_LEDS;
    data->rightPixelsBlock = data->height / AmbientLight::RIGHT_LEDS;

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

PIXEL_DATA AmbientLight::getColorFromArea(const int16_t &x1, const int16_t &x2,
                                          const int16_t &y1, const int16_t &y2,
                                          const uint8_t *pixels,
                                          const AmbientLightData *userData) {
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
void AmbientLight::on_process(void *userdata) {
    AmbientLightData *data = (AmbientLightData *)userdata;
    struct pw_buffer *b;
    struct spa_buffer *buf;

    if ((b = pw_stream_dequeue_buffer(data->pw_stream)) == NULL) {
        return; // no ready buffer;
    }

    buf = b->buffer;

    unsigned int x = data->width / 2, y = data->height / 2;
    if (buf->datas[0].data != NULL || data->initialized) {
        uint8_t *p = (uint8_t *)buf->datas[0].data;
        uint16_t led_id = 0;
        int16_t i = 0;

        PIXEL_DATA pd;

        // std::cout << "BLOCK R T L " << data->rightPixelsBlock << " "
        //           << data->topPixelsBlock << " " << data->leftPixelsBlock
        //           << std::endl;

        // pd = AmbientLight::getColorFromArea(1820, 1919, 0, 77, p, data);

        for (i = AmbientLight::RIGHT_LEDS - 1; i > -1; --i) {
            // std::cout << (int)i << ". ";
            pd = AmbientLight::getColorFromArea(
                data->width - AmbientLight::PADDING, data->width - 1,
                i * data->rightPixelsBlock,
                i * data->rightPixelsBlock + data->rightPixelsBlock, p, data);
            data->leds[led_id].setColor(pd.r, pd.g, pd.b);
            led_id++;
        }

        for (i = AmbientLight::TOP_LEDS - 1; i > -1; --i) {
            pd = AmbientLight::getColorFromArea(
                i * data->topPixelsBlock,
                i * data->topPixelsBlock + data->topPixelsBlock - 1, 1,
                AmbientLight::PADDING, p, data);
            data->leds[led_id].setColor(pd.r, pd.g, pd.b);
            led_id++;
        }

        for (i = 0; i < AmbientLight::LEFT_LEDS; ++i) {
            pd = AmbientLight::getColorFromArea(
               1, 
                AmbientLight::PADDING, i * data->leftPixelsBlock,
                i * data->leftPixelsBlock + data->leftPixelsBlock  - 1, p, data);
            data->leds[led_id].setColor(pd.r, pd.g, pd.b);
            led_id++;
        }
    }

    pw_stream_queue_buffer(data->pw_stream, b);
}

void AmbientLight::start_pipewire(const uint32_t &node_id,
                                  AmbientLightData *data) {
    std::cout << "PipeWire: Starting thread loop..." << std::endl;
    data->pw_thread_loop = pw_thread_loop_new("PixelReaderLoop", NULL);

    data->pw_context =
        pw_context_new(pw_thread_loop_get_loop(data->pw_thread_loop), NULL, 0);

    pw_thread_loop_lock(data->pw_thread_loop);

    data->pw_core = pw_context_connect(data->pw_context, NULL, 0);

    if (!data->pw_core) {
        std::cerr << "PipeWire: Failed to connect to core." << std::endl;
        return;
    }

    data->pw_stream = pw_stream_new(
        data->pw_core, "Pixel Reader",
        pw_properties_new(PW_KEY_MEDIA_TYPE, "Video", PW_KEY_MEDIA_CATEGORY,
                          "Capture", NULL));

    static const struct pw_stream_events stream_events = {
        PW_VERSION_STREAM_EVENTS,
        .state_changed = AmbientLight::on_state_changed,
        .param_changed = AmbientLight::on_param_changed,
        .process = on_process,
    };

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

    pw_thread_loop_unlock(data->pw_thread_loop);

    std::cout << "PipeWire: Starting processing thread..." << std::endl;
    pw_thread_loop_start(data->pw_thread_loop);
}

void AmbientLight::on_screencast_start(GObject *source_object,
                                       GAsyncResult *res, gpointer user_data) {
    AmbientLightData *data = (AmbientLightData *)user_data;

    XdpSession *session = (XdpSession *)source_object;
    GError *error = NULL;

    if (xdp_session_start_finish(session, res, &error)) {
        std::cout << "Session started by user! Looking for ID.." << std::endl;
        RESTORE_TOKEN = xdp_session_get_restore_token(session);
        std::cout << "TOKEN: " << RESTORE_TOKEN << std::endl;

        GVariant *streams = xdp_session_get_streams(session);

        if (streams) {
            GVariantIter iter;
            g_variant_iter_init(&iter, streams);
            uint32_t node_id;
            GVariant *options = NULL;

            if (g_variant_iter_next(&iter, "(u@a{sv})", &node_id, &options)) {
                std::cout << "Portal: Success! Node ID: " << node_id
                          << std::endl;

                AmbientLight::start_pipewire(node_id, data);
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

void AmbientLight::on_screencast_created(GObject *source_object,
                                         GAsyncResult *res,
                                         gpointer user_data) {
    AmbientLightData *data = (AmbientLightData *)user_data;
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

    xdp_session_start(session, NULL, NULL, AmbientLight::on_screencast_start,
                      user_data);
}

AmbientLight::AmbientLight(uint8_t topLeds, uint8_t rightLeds, uint8_t leftLeds,
                           uint16_t padding) {
    AmbientLight::TOP_LEDS = topLeds;
    AmbientLight::RIGHT_LEDS = rightLeds;
    AmbientLight::LEFT_LEDS = leftLeds;
    AmbientLight::PADDING = padding;

    pw_init(NULL, NULL);
    m_data = new AmbientLightData();
    m_data->initialized = false;

    m_workerThread = std::thread(&AmbientLight::workerThread, this);
}

AmbientLight::~AmbientLight() {
    std::cout << "stop everything\n";

    if (m_data->pw_thread_loop) {
        pw_thread_loop_stop(m_data->pw_thread_loop);
        pw_thread_loop_destroy(m_data->pw_thread_loop);
    }

    if (m_data->loop) {
        g_main_loop_quit(m_data->loop);
    }

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    m_data->leds = nullptr;
    delete m_data;
}

void AmbientLight::workerThread() {
    m_data->loop = g_main_loop_new(NULL, FALSE);
    m_data->portal = xdp_portal_new();

    if (RESTORE_TOKEN != "") {
        xdp_portal_create_screencast_session(
            m_data->portal, XDP_OUTPUT_MONITOR, XDP_SCREENCAST_FLAG_NONE,
            XDP_CURSOR_MODE_EMBEDDED, XDP_PERSIST_MODE_PERSISTENT,
            RESTORE_TOKEN, NULL, AmbientLight::on_screencast_created, m_data);
    } else {
        xdp_portal_create_screencast_session(
            m_data->portal, XDP_OUTPUT_MONITOR, XDP_SCREENCAST_FLAG_NONE,
            XDP_CURSOR_MODE_EMBEDDED, XDP_PERSIST_MODE_PERSISTENT, NULL, NULL,
            AmbientLight::on_screencast_created, m_data);
    }

    g_main_loop_run(m_data->loop);
    if (m_data->portal)
        g_object_unref(m_data->portal);
    if (m_data->loop)
        g_main_loop_unref(m_data->loop);
}

bool AmbientLight::update() {
    if (m_data->initialized == false) {
        m_data->leds = outputArr;

        for (uint16_t i = 0;
             i < AmbientLight::TOP_LEDS + AmbientLight::LEFT_LEDS +
                     AmbientLight::RIGHT_LEDS;
             ++i) {
            m_data->leds[i].setId(i);
        }
        m_data->initialized = true;
    }
    return false;
}
