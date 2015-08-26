#include <pebble.h>
#include "dithered_rects.h"

#define KEY_NOW 0
#define KEY_NEXT 1

static Window *window;
static TextLayer *timeText;
static TextLayer *now_layer;
static TextLayer *next_layer;
static Layer* background_layer;

static GColor colors[7] = {GColorBabyBlueEyesARGB8, GColorIndigoARGB8, GColorLightGrayARGB8, GColorChromeYellowARGB8, GColorPictonBlueARGB8, GColorCelesteARGB8, GColorCadetBlueARGB8};
static char *english[7] = {"cloudy","rain","snow","sun","clear","foggy","windy"};
static int8_t code_mapping[48]={6,6,6,6,6,2,1,2,1,1,1,1,1,2,2,2,2,2,2,5,5,5,5,6,6,0,0,0,0,0,0,4,3,4,4,1,3,6,6,6,1,2,2,2,0,1,2,1};
static int now_code=0;
static int next_code=0;

static void updateTime() {
    time_t currentTime = time(NULL);
    struct tm *tick = localtime(&currentTime);

    static char timeBuffer[] = "00:00";

    if (clock_is_24h_style() == true) {
        strftime(timeBuffer, sizeof("00:00"), "%H:%M", tick);
    } else {
        strftime(timeBuffer, sizeof("00:00"), "%I:%M", tick);
    }

    text_layer_set_text(timeText, timeBuffer);
}


static void background_update_proc(Layer *layer, GContext *ctx) {

    draw_smooth_gradient_rect(ctx, GRect(0,0,144,168), colors[code_mapping[now_code]], colors[code_mapping[next_code]], TOP_TO_BOTTOM);
}

static void loadWindow(Window *window) {
    background_layer = layer_create(GRect(0,0,144,168));
    layer_set_update_proc(background_layer, background_update_proc);


    timeText = text_layer_create(GRect(0, 0, 144, 30));
    text_layer_set_background_color(timeText, GColorClear);
    text_layer_set_text_color(timeText, GColorWhite);
    text_layer_set_font(timeText, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
    text_layer_set_text_alignment(timeText, GTextAlignmentCenter);

    now_layer = text_layer_create(GRect(0, 31, 144, 38));
    text_layer_set_background_color(now_layer, GColorClear);
    text_layer_set_text_color(now_layer, GColorWhite);
    text_layer_set_font(now_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
    text_layer_set_text_alignment(now_layer, GTextAlignmentCenter);
    text_layer_set_text(now_layer, " ");

    next_layer = text_layer_create(GRect(0, 168-38, 144, 38));
    text_layer_set_background_color(next_layer, GColorClear);
    text_layer_set_text_color(next_layer, GColorWhite);
    text_layer_set_font(next_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
    text_layer_set_text_alignment(next_layer, GTextAlignmentCenter);
    text_layer_set_text(next_layer, " ");

    layer_add_child(window_get_root_layer(window), background_layer);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(timeText));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(now_layer));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(next_layer));
}

static void unloadWindow(Window *window) {
    text_layer_destroy(timeText);
    text_layer_destroy(now_layer);
    text_layer_destroy(next_layer);
}

static void tickHandler(struct tm *tick, TimeUnits units) {
    updateTime();

    if (tick->tm_min % 15 == 0) {
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);
        dict_write_uint8(iter, 0, 0);
        app_message_outbox_send();
        layer_mark_dirty(background_layer);
    }
}

static void receivedCallback(DictionaryIterator *iterator, void *context) {
    static char nowBuffer[20];
    static char nextBuffer[20];

    Tuple *tuple = dict_read_first(iterator);

    while (tuple != NULL) {

        switch (tuple->key) {

            case KEY_NOW:
                now_code = (int)tuple->value->int32;
                snprintf(nowBuffer, sizeof(nowBuffer), "%s",
                    english[code_mapping[now_code]]);

                break;
                
            case KEY_NEXT:
                next_code = (int)tuple->value->int32;
                snprintf(nextBuffer, sizeof(nextBuffer), "%s",
                    english[code_mapping[next_code]]);
                break;

            default:
                break;
        }

        tuple = dict_read_next(iterator);
    }

    text_layer_set_text(now_layer,  nowBuffer);
    text_layer_set_text(next_layer, nextBuffer);
}

static void droppedCallback(AppMessageResult reason, void *context) {
}

static void failedCallback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
}

static void sentCallback(DictionaryIterator *iterator, void *context) {
}

static void init() {
    window = window_create();

    window_set_window_handlers(window, (WindowHandlers) {
        .load = loadWindow,
        .unload = unloadWindow
    });

    window_stack_push(window, true);

    updateTime();

    tick_timer_service_subscribe(MINUTE_UNIT, tickHandler);

    app_message_register_inbox_received(receivedCallback);
    app_message_register_inbox_dropped(droppedCallback);
    app_message_register_outbox_failed(failedCallback);
    app_message_register_outbox_sent(sentCallback);

    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
