#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>
#include <zmk/keys.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/input_processor.h>
#include <stdlib.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* 閾値の定義。センサの値がこれ以上ならキー入力を発行 */
#define ARROW_THRESHOLD 10

/* X軸とY軸それぞれで、どのキーが押下中かを保持 */
static bool arrow_active_x = false;
static bool arrow_active_y = false;
static uint32_t current_arrow_x = 0;
static uint32_t current_arrow_y = 0;

/*
 * この関数は、相対センサの入力イベント（evt）を受け取り、
 * X軸の場合は RIGHT_ARROW/LEFT_ARROW、Y軸の場合は DOWN_ARROW/UP_ARROW の
 * プレス／リリースイベントを発行します。
 *
 * ※ evt->axis や evt->value のフィールドは、実装環境に合わせて変更してください。
 */
static int custom_arrow_processor_process(const struct device *dev,　struct zmk_input_event *evt,
                                          void *context) {
    int16_t delta = (int16_t)evt->value;

    if (evt->axis == INPUT_REL_X) {
        if (abs(delta) >= ARROW_THRESHOLD) {
            uint32_t key = (delta > 0) ? RIGHT_ARROW : LEFT_ARROW;
            if (!arrow_active_x || (current_arrow_x != key)) {
                /* すでに別の X 軸キーが押されているなら解放 */
                if (arrow_active_x) {
                    struct zmk_event_keycode_state_changed release = {
                        .keycode = current_arrow_x,
                        .state = false,
                    };
                    ZMK_EVENT_RAISE(new_keycode_state_changed, &release);
                }
                /* 新しいキーを押下 */
                struct zmk_event_keycode_state_changed press = {
                    .keycode = key,
                    .state = true,
                };
                ZMK_EVENT_RAISE(new_keycode_state_changed, &press);
                arrow_active_x = true;
                current_arrow_x = key;
            }
        } else {
            if (arrow_active_x) {
                /* 閾値以下なら解放 */
                struct zmk_event_keycode_state_changed release = {
                    .keycode = current_arrow_x,
                    .state = false,
                };
                ZMK_EVENT_RAISE(new_keycode_state_changed, &release);
                arrow_active_x = false;
                current_arrow_x = 0;
            }
        }
    } else if (evt->axis == INPUT_REL_Y) {
        if (abs(delta) >= ARROW_THRESHOLD) {
            uint32_t key = (delta > 0) ? DOWN_ARROW : UP_ARROW;
            if (!arrow_active_y || (current_arrow_y != key)) {
                if (arrow_active_y) {
                    struct zmk_event_keycode_state_changed release = {
                        .keycode = current_arrow_y,
                        .state = false,
                    };
                    ZMK_EVENT_RAISE(new_keycode_state_changed, &release);
                }
                struct zmk_event_keycode_state_changed press = {
                    .keycode = key,
                    .state = true,
                };
                ZMK_EVENT_RAISE(new_keycode_state_changed, &press);
                arrow_active_y = true;
                current_arrow_y = key;
            }
        } else {
            if (arrow_active_y) {
                struct zmk_event_keycode_state_changed release = {
                    .keycode = current_arrow_y,
                    .state = false,
                };
                ZMK_EVENT_RAISE(new_keycode_state_changed, &release);
                arrow_active_y = false;
                current_arrow_y = 0;
            }
        }
    }
    return 0;
}

/* 入力プロセッサの API 構造体 */
static const struct zmk_input_processor_driver_api custom_arrow_processor_driver_api = {
    .process = custom_arrow_processor_process,
};

/* デバイス定義。compatible 文字列は DTS で参照する際に使用します。 */
DEVICE_DEFINE(custom_arrow_mapper, "zmk,input-processor-custom-arrow",
              custom_arrow_processor_process, NULL, NULL, POST_KERNEL,
              CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &custom_arrow_processor_driver_api);
