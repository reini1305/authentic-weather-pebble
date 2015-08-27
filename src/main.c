#include <pebble.h>
#include "dithered_rects.h"

#define KEY_NOW 0
#define KEY_NEXT 1

static Window *window;
static TextLayer *timeText;
static TextLayer *now_layer;
static TextLayer *next_layer;
static TextLayer *then_layer;
static Layer* background_layer;

static GColor colors[7];
static char *english[8] = {"clouds","rain","snow","sun","clear","fog","wind","then"};
static char *german[8] = {"Wolken","Regen","Schnee","Sonne","Klar","Nebel","Wind","dann"};
static char **language;
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
    if(code_mapping[now_code] == code_mapping[next_code]) {
      graphics_context_set_fill_color(ctx,colors[code_mapping[now_code]]);
      graphics_fill_rect(ctx,GRect(0,0,144,168),0,GCornerNone);
    }
    else
      draw_smooth_gradient_rect(ctx, GRect(0,0,144,169), colors[code_mapping[now_code]], colors[code_mapping[next_code]], TOP_TO_BOTTOM);
}

static void loadWindow(Window *window) {
    background_layer = layer_create(GRect(0,0,144,168));
    layer_set_update_proc(background_layer, background_update_proc);


    timeText = text_layer_create(GRect(0, 0, 144, 30));
    text_layer_set_background_color(timeText, GColorClear);
    text_layer_set_text_color(timeText, GColorBlack);
    text_layer_set_font(timeText, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
    text_layer_set_text_alignment(timeText, GTextAlignmentCenter);

    now_layer = text_layer_create(GRect(0, 27, 144, 50));
    text_layer_set_background_color(now_layer, GColorClear);
    text_layer_set_text_color(now_layer, GColorWhite);
    text_layer_set_font(now_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
    text_layer_set_text_alignment(now_layer, GTextAlignmentCenter);
    text_layer_set_text(now_layer, " ");
  
    then_layer = text_layer_create(GRect(0, 72, 144, 50));
    text_layer_set_background_color(then_layer, GColorClear);
    text_layer_set_text_color(then_layer, GColorWhite);
    text_layer_set_font(then_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
    text_layer_set_text_alignment(then_layer, GTextAlignmentCenter);
    text_layer_set_text(then_layer, " ");

    next_layer = text_layer_create(GRect(0, 168-50, 144, 50));
    text_layer_set_background_color(next_layer, GColorClear);
    text_layer_set_text_color(next_layer, GColorWhite);
    text_layer_set_font(next_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
    text_layer_set_text_alignment(next_layer, GTextAlignmentCenter);
    text_layer_set_text(next_layer, " ");

    layer_add_child(window_get_root_layer(window), background_layer);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(timeText));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(now_layer));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(then_layer));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(next_layer));
  
    updateTime();
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
                text_layer_set_text_color(timeText, GColorWhite);
                now_code = (int)tuple->value->int32;
                snprintf(nowBuffer, sizeof(nowBuffer), "%s",
                    language[code_mapping[now_code]]);

                break;
                
            case KEY_NEXT:
                next_code = (int)tuple->value->int32;
                snprintf(nextBuffer, sizeof(nextBuffer), "%s",
                    language[code_mapping[next_code]]);
                break;

            default:
                break;
        }

        tuple = dict_read_next(iterator);
    }

    text_layer_set_text(now_layer,  nowBuffer);
    text_layer_set_text(next_layer, nextBuffer);
    text_layer_set_text(then_layer,language[7]);
    layer_mark_dirty(background_layer);
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
  
    colors[0] = GColorBabyBlueEyes;
    colors[1] = GColorIndigo;
    colors[2] = GColorLightGray;
    colors[3] = GColorChromeYellow;
    colors[4] = GColorPictonBlue;
    colors[5] = GColorCeleste;
    colors[6] = GColorCadetBlue;
  
    char *sys_locale = setlocale(LC_ALL, "");
    if (strcmp("de_DE", sys_locale) == 0) {
      language = german;
    }
    else{
      language = english;
    }

    window_stack_push(window, true);

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
