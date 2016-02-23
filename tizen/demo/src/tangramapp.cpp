#include "tangramapp.h"

#include "platform_gl.h"
#include "platform_tizen.h"

#include "tangram.h"
#include "debug.h"

typedef struct appdata {
    Evas_Object *win;
    Evas_Object *conform;
    Evas_Object *glview;
} appdata_s;

double _time = -1;

static void _draw_gl(Evas_Object *obj) {
    ELEMENTARY_GLVIEW_GLOBAL_USE(obj);

    float t = 0;
    double time = ecore_time_get();
    if (_time > 0) { t = time - _time; }
    _time = time;

    Tangram::update(t);
    Tangram::render();
}

static void _resize_gl(Evas_Object *obj) {
    LOG("RESIZE GL");

    ELEMENTARY_GLVIEW_GLOBAL_USE(obj);

    int w, h;
    elm_glview_size_get(obj, &w, &h);
    if (w > 0 && h > 0) {
        Tangram::resize(w, h);
    }
}

static void _init_gl(Evas_Object *obj) {
    LOG("INIT GL");

    ELEMENTARY_GLVIEW_GLOBAL_USE(obj);

    Tangram::initialize("scene.yaml");

    Tangram::setupGL();

    ecore_timer_add(0.1, [](void *data) {
            bool anim = shouldRender();
            if (anim) {
                elm_glview_changed_set((Elm_Glview*)data);
            }
            return ECORE_CALLBACK_RENEW;
        }, obj);
}

Evas_Object* glview_create(Evas_Object *win) {
    Evas_Object *glview;

    glview = elm_glview_add(win);
    elm_win_resize_object_add(win, glview);
    elm_glview_mode_set(glview, static_cast<Elm_GLView_Mode>(ELM_GLVIEW_DEPTH |
                                                             ELM_GLVIEW_STENCIL));

    elm_glview_resize_policy_set(glview, ELM_GLVIEW_RESIZE_POLICY_RECREATE);
    elm_glview_render_policy_set(glview, ELM_GLVIEW_RENDER_POLICY_ON_DEMAND);
    //elm_glview_render_policy_set(glview, ELM_GLVIEW_RENDER_POLICY_ALWAYS);
    elm_glview_init_func_set(glview, _init_gl);
    //elm_glview_del_func_set(glview, _del_gl);
    elm_glview_render_func_set(glview, _draw_gl);
    elm_glview_resize_func_set(glview, _resize_gl);
    //evas_object_size_hint_min_set(glview, 800, 600);
    //elm_glview_size_set(glview, 800, 600);
    evas_object_show(glview);

    return glview;
}

bool panning = false;
double last_x_down = 0.0;
double last_y_down = 0.0;

static void
_mouse_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info) {
    auto ev = reinterpret_cast<Evas_Event_Mouse_Down *>(event_info);
    panning = true;
    last_x_down = ev->canvas.x;
    last_y_down = ev->canvas.y;
}

static void
_mouse_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info) {
    auto ev = reinterpret_cast<Evas_Event_Mouse_Up *>(event_info);
    panning = false;
}

static void
_mouse_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info) {
    auto ev = reinterpret_cast<Evas_Event_Mouse_Move *>(event_info);

    if (!panning) return;

    //if (was_panning) {
    Tangram::handlePanGesture(last_x_down, last_y_down, ev->cur.canvas.x, ev->cur.canvas.y);
    //}

    // last_x_velocity = (x - last_x_down) / (time - last_time_moved);
    // last_y_velocity = (y - last_y_down) / (time - last_time_moved);
    last_x_down = ev->cur.canvas.x;
    last_y_down = ev->cur.canvas.y;

    elm_glview_changed_set((Elm_Glview*)data);

}

static void
_mouse_wheel_cb(void *data, Evas *e, Evas_Object *obj, void *event_info) {
    auto ev = reinterpret_cast<Evas_Event_Mouse_Wheel *>(event_info);
    const double scroll_span_multiplier = 0.05; // scaling for zoom and rotation

    Tangram::handlePinchGesture(ev->canvas.x, ev->canvas.y, 1.0 - scroll_span_multiplier * ev->z, 0.f);

    auto glview = reinterpret_cast<Elm_Glview*>(data);

    elm_glview_changed_set((Elm_Glview*)data);

}


static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info) {
    ui_app_exit();
}

static void
win_back_cb(void *data, Evas_Object *obj, void *event_info) {
    appdata_s *ad = (appdata_s*)data;
    /* Let window go to hide state. */
    elm_win_lower(ad->win);
}

static void
create_base_gui(appdata_s *ad) {
    /* Window */
    /* Create and initialize elm_win.
       elm_win is mandatory to manipulate window. */

    elm_config_accel_preference_set("opengl");

    ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
    elm_win_autodel_set(ad->win, EINA_TRUE);

    if (elm_win_wm_rotation_supported_get(ad->win)) {
        int rots[4] = { 0, 90, 180, 270 };
        elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 4);
    }

    evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);
    eext_object_event_callback_add(ad->win, EEXT_CALLBACK_BACK, win_back_cb, ad);

    /* Conformant */
    /* Create and initialize elm_conformant.
       elm_conformant is mandatory for base gui to have proper size
       when indicator or virtual keypad is visible. */
    ad->conform = elm_conformant_add(ad->win);
    elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
    elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_OPAQUE);
    evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(ad->win, ad->conform);
    evas_object_show(ad->conform);



    auto obj = glview_create(ad->conform);
    //elm_win_resize_object_add(win, obj);

    evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_DOWN,
                                   _mouse_down_cb, obj);
    evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb,
                                   obj);
    evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_MOVE,
                                   _mouse_move_cb, obj);
    evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_WHEEL,
                                   _mouse_wheel_cb, obj);

    ad->glview = obj;
    evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

    elm_object_content_set(ad->conform, obj);
    elm_object_focus_set(obj, EINA_TRUE);


    /* Show window after base gui is set up */
    evas_object_show(ad->win);

}

static bool
app_create(void *data) {
    appdata_s *ad = (appdata_s*)data;
    create_base_gui(ad);

    initUrlRequests();

    return true;
}

static void
app_control(app_control_h app_control, void *data) {
    /* Handle the launch request. */
}

static void
app_pause(void *data) {
    /* Take necessary actions when application becomes invisible. */
}

static void
app_resume(void *data) {
    /* Take necessary actions when application becomes visible. */
}

static void
app_terminate(void *data) {
    /* Release all resources. */
}

static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data) {
    char *locale = NULL;
    system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
    elm_language_set(locale);
    free(locale);
    return;
}

static void
ui_app_orient_changed(app_event_info_h event_info, void *user_data) {
    return;
}

static void
ui_app_region_changed(app_event_info_h event_info, void *user_data) {
}

static void
ui_app_low_battery(app_event_info_h event_info, void *user_data) {
}

static void
ui_app_low_memory(app_event_info_h event_info, void *user_data) {
}

int
main(int argc, char *argv[]) {

    appdata_s ad = {0,};
    int ret = 0;

    ui_app_lifecycle_callback_s event_callback = {0,};
    app_event_handler_h handlers[5] = {NULL, };

    event_callback.create = app_create;
    event_callback.terminate = app_terminate;
    event_callback.pause = app_pause;
    event_callback.resume = app_resume;
    event_callback.app_control = app_control;

    ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
    ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
    ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
    ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
    ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);

    ret = ui_app_main(argc, argv, &event_callback, &ad);
    if (ret != APP_ERROR_NONE) {
        dlog_print(DLOG_ERROR, LOG_TAG, "app_main() is failed. err = %d", ret);
    }

    return ret;
}
