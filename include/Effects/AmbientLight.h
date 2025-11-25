#pragma once

#include <gio/gio.h>
#include <glib.h>
#include <libportal/portal.h>
#include <libportal/remote.h>
#include <libportal/session.h>

#include <pipewire/pipewire.h>
#include <pipewire/thread-loop.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/video/raw.h>

#include <thread>

#include <Effect.h>

struct AmbientLightData {
    GMainLoop *loop;
    XdpPortal *portal;

    struct pw_thread_loop *pw_thread_loop;
    struct pw_context *pw_context;
    struct pw_core *pw_core;
    struct pw_stream *pw_stream;
    struct spa_hook stream_listener;

    int width;
    int height;
    int stride;
    int bpp;

    uint16_t leftPixelsBlock;
    uint16_t rightPixelsBlock;
    uint16_t topPixelsBlock;

    int red_offset;
    int blue_offset;
    int green_offset;

    Pixel* leds;
    bool initialized;
};

struct PIXEL_DATA {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

static const char *RESTORE_TOKEN = "";

class AmbientLight : public Effect {
  private:
    static uint8_t TOP_LEDS;
    static uint8_t LEFT_LEDS;
    static uint8_t RIGHT_LEDS;
    static uint16_t PADDING;
    bool initialized = false;
    AmbientLightData *m_data;

    std::thread m_workerThread;

    static PIXEL_DATA getColorFromArea(const int16_t &x1, const int16_t &x2,
                                       const int16_t &y1, const int16_t &y2,
                                       const uint8_t *pixels,
                                       const AmbientLightData *userData);
    void workerThread();

    static void on_process(void *userdata);
    static void on_state_changed(void *userdata, enum pw_stream_state old,
                                 enum pw_stream_state state, const char *error);
    static void on_param_changed(void *userdata, uint32_t id,
                                 const struct spa_pod *param);
    static void start_pipewire(const uint32_t &node_id, AmbientLightData *data);
    static void on_screencast_start(GObject *source_object, GAsyncResult *res,
                                    gpointer user_data);
    static void on_screencast_created(GObject *source_object, GAsyncResult *res,
                                      gpointer user_data);

  public:
    void init();
    bool update();
    AmbientLight(uint8_t topLeds, uint8_t rightLeds, uint8_t leftLeds, uint16_t padding);
    ~AmbientLight();
};
