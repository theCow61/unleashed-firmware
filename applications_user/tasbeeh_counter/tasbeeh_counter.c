#include <furi.h>
#include <stdlib.h>
#include <input/input.h>
#include <gui/gui.h>
#include <furi/core/string.h>
#include <math.h>
#include <dialogs/dialogs.h>
#include <gui/elements.h>

typedef struct {
    u_int32_t count;
    // char* count_str;
    bool reset_highlighted;

    ViewPort* vp;
    Gui* gui;

    FuriMessageQueue* message_queue;

} TasbeehCounter;

// 128x64 px
void draw_callback(Canvas* canvas, void* ctx) {
    furi_assert(ctx);
    TasbeehCounter* instance = ctx;

    size_t size = log10(instance->count) + 2;
    char f_text[size];
    snprintf(f_text, sizeof(f_text), "%ld", instance->count);

    // use elements

    canvas_set_font(canvas, FontPrimary);
    // canvas_draw_str(canvas, 128 / 2, 64 / 2, f_text);
    canvas_draw_str_aligned(canvas, 128 / 2, 64 / 2, AlignCenter, AlignCenter, f_text);

    free(f_text);

    canvas_draw_str(canvas, 50, 60, "Reset");
    canvas_draw_rframe(canvas, 47, 49, 35, 14, 2);

    if(instance->reset_highlighted) {
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_rbox(canvas, 47, 49, 35, 13, 2);
        // canvas_draw_frame(canvas, 48, 50, 33, 12);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_str(canvas, 50, 60, "Reset");
    }
}

void input_callback(InputEvent* evt, void* ctx) {
    furi_assert(ctx);
    TasbeehCounter* instance = ctx;
    furi_message_queue_put(instance->message_queue, evt, FuriWaitForever);
}

TasbeehCounter* tasbeeh_counter_alloc() {
    TasbeehCounter* instance = malloc(sizeof(TasbeehCounter));
    instance->count = 0;
    instance->reset_highlighted = false;

    instance->vp = view_port_alloc();
    view_port_enabled_set(instance->vp, true);
    view_port_draw_callback_set(instance->vp, draw_callback, instance);
    view_port_input_callback_set(instance->vp, input_callback, instance);

    instance->gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(instance->gui, instance->vp, GuiLayerFullscreen);

    instance->message_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    return instance;
}

void tasbeeh_counter_free(TasbeehCounter* instance) {
    view_port_enabled_set(instance->vp, false);
    gui_remove_view_port(instance->gui, instance->vp);
    view_port_free(instance->vp);
    furi_record_close(RECORD_GUI);

    furi_message_queue_free(instance->message_queue);

    free(instance);
}

int32_t tasbeeh_counter_main(void* p) {
    UNUSED(p);

    TasbeehCounter* instance = tasbeeh_counter_alloc();

    bool running = true;
    InputEvent event;

    while(running) {
        if(furi_message_queue_get(instance->message_queue, &event, FuriWaitForever) ==
           FuriStatusOk) {
            if(event.type == InputTypePress) {
                switch(event.key) {
                case InputKeyUp:
                case InputKeyDown:
                case InputKeyRight:
                case InputKeyLeft:
                    if(instance->reset_highlighted) {
                        instance->reset_highlighted = false;
                        break;
                    }
                    instance->reset_highlighted = true;
                    break;
                case InputKeyOk:
                    if(instance->reset_highlighted) {
                        instance->count = 0;
                        break;
                    }
                    instance->count++;
                    break;
                default:
                case InputKeyBack:
                    running = false;
                    break;
                }
            }
        }

        view_port_update(instance->vp);
    }

    tasbeeh_counter_free(instance);
    return 0;
}