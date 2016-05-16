#include "tangramapp.h"

#include "platform_gl.h"
#include "platform_tizen.h"

#include "tangram.h"

#define TAG "Tangram"

#define DEG_TO_RAD(x) (x) * (M_PI / 180.0)

typedef struct appdata {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *glview;

	Ecore_Animator *animator;
	char path[512];

	struct {
		double angle;
		double zoom;
		Evas_Coord last_x;
		Evas_Coord last_y;
		bool rotation;
		bool zooming;
		bool can_shove;
		bool shoving;
		bool panning;
	} event;

} appdata_s;

double _time = -1;

static void _draw_gl(Evas_Object *obj) {
	ELEMENTARY_GLVIEW_GLOBAL_USE(obj);

	float t = 0;
	double time = ecore_time_get();
	if (_time > 0) {
		t = time - _time;
	}
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
	//Tangram::setPixelScale(2);

	Tangram::setupGL();

	Tangram::toggleDebugFlag(Tangram::DebugFlags::tangram_infos);

	ecore_timer_add(0.01, [](void *data) {
		bool anim = shouldRender();
		if (anim) {
			elm_glview_changed_set((Evas_Object*)data);
		}
		return ECORE_CALLBACK_RENEW;
	}, obj);
}

static Evas_Event_Flags zoom_start(void *data, void *event_info) {
	Elm_Gesture_Zoom_Info *p = (Elm_Gesture_Zoom_Info *) event_info;
	LOGD("zoom started <%d,%d> zoom=<%f> radius=<%d> momentum=<%f>\n", p->x,
			p->y, p->zoom, p->radius, p->momentum);

	auto ad = reinterpret_cast<appdata*>(data);
	ad->event.zooming = false;
	ad->event.shoving = false;

	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags zoom_move(void *data, void *event_info) {
	Elm_Gesture_Zoom_Info *p = (Elm_Gesture_Zoom_Info *) event_info;
	auto ad = reinterpret_cast<appdata*>(data);

	if (ad->event.shoving) {
		return EVAS_EVENT_FLAG_ON_HOLD;
	}

	if (!ad->event.zooming) {
		ad->event.zooming = true;
		ad->event.zoom = p->zoom;
		return EVAS_EVENT_FLAG_ON_HOLD;
	}

	LOGD("zoom move <%d,%d> zoom=<%f> radius=<%d> momentum=<%f>\n", p->x, p->y,
			p->zoom, p->radius, p->momentum);

	Tangram::handlePinchGesture(p->x, p->y, 1 + (p->zoom - ad->event.zoom), 0);
	ad->event.zoom = p->zoom;

	elm_glview_changed_set(ad->glview);

	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags zoom_end(void *data, void *event_info) {
	Elm_Gesture_Zoom_Info *p = (Elm_Gesture_Zoom_Info *) event_info;
	LOGD("zoom end <%d,%d> zoom=<%f> radius=<%d> momentum=<%f>\n", p->x,
			p->y, p->zoom, p->radius, p->momentum);

	auto ad = reinterpret_cast<appdata*>(data);
	if (ad->event.zooming) {
		ad->event.zooming = false;

		Tangram::handlePinchGesture(p->x, p->y, 1 + (p->zoom - ad->event.zoom),
				p->momentum);

		elm_glview_changed_set(ad->glview);
		// stop panning to allow pinch animation to run
		ad->event.panning = false;
	}
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags zoom_abort(void *data, void *event_info EINA_UNUSED) {
	LOGD("zoom abort\n");
	auto ad = reinterpret_cast<appdata*>(data);
	ad->event.zooming = false;
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags rotate_start(void *data, void *event_info) {
	Elm_Gesture_Rotate_Info *p = (Elm_Gesture_Rotate_Info *) event_info;
	LOGD("rotate started <%d,%d> base=<%f> angle=<%f> radius=<%d> momentum=<%f>\n",
			p->x, p->y, p->base_angle, p->angle, p->radius, p->momentum);

	auto ad = reinterpret_cast<appdata*>(data);
	ad->event.rotation = false;
	ad->event.shoving = false;

	ad->event.can_shove =
		((p->base_angle > 70 && p->base_angle < 110)
		|| (p->base_angle > 250 && p->base_angle < 290));

	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags rotate_abort(void *data, void *event_info EINA_UNUSED) {
	LOGD("rotate abort\n");
	auto ad = reinterpret_cast<appdata*>(data);
	ad->event.rotation = false;
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags rotate_move(void *data, void *event_info) {
	Elm_Gesture_Rotate_Info *p = (Elm_Gesture_Rotate_Info *) event_info;
	auto ad = reinterpret_cast<appdata*>(data);

	if (ad->event.shoving) {
		return EVAS_EVENT_FLAG_ON_HOLD;
	}

	if (!ad->event.rotation) {
		ad->event.rotation = true;
		ad->event.angle = p->angle;
		return EVAS_EVENT_FLAG_ON_HOLD;
	}
	LOGD("rotate move <%d,%d> base=<%f> angle=<%f> radius=<%d> momentum=<%f> => d:%f",
			p->x, p->y, p->base_angle, p->angle, p->radius, p->momentum,
			DEG_TO_RAD(p->angle - ad->event.angle));

	Tangram::handleRotateGesture(p->x, p->y,
			DEG_TO_RAD(p->angle - ad->event.angle));
	ad->event.angle = p->angle;

	elm_glview_changed_set(ad->glview);
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags rotate_end(void *data, void *event_info) {
	Elm_Gesture_Rotate_Info *p = (Elm_Gesture_Rotate_Info *) event_info;
	LOGD("rotate end <%d,%d> base=<%f> angle=<%f> radius=<%d> momentum=<%f>\n",
			p->x, p->y, p->base_angle, p->angle, p->radius, p->momentum);

	auto ad = reinterpret_cast<appdata*>(data);
	ad->event.rotation = false;
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags momentum_start(void *data, void *event_info) {
	Elm_Gesture_Momentum_Info *p = (Elm_Gesture_Momentum_Info *) event_info;
	LOGD("momentum started x1,y1=<%d,%d> tx,ty=<%d,%d> n=<%u>\n", p->x1,
			p->y1, p->tx, p->ty, p->n);

	auto ad = reinterpret_cast<appdata*>(data);
	ad->event.last_x = p->x1;
	ad->event.last_y = p->y1;

	ad->event.shoving = false;
	ad->event.can_shove = false;
	ad->event.panning = false;

	return EVAS_EVENT_FLAG_ON_HOLD;
}
static Evas_Event_Flags momentum_move(void *data, void *event_info) {
	Elm_Gesture_Momentum_Info *p = (Elm_Gesture_Momentum_Info *) event_info;
	LOGD("momentum move x1,y1=<%d,%d> x2,y2=<%d,%d> tx,ty=<%d,%d> mx=<%d> my=<%d> n=<%u>\n",
			p->x1, p->y1, p->x2, p->y2, p->tx, p->ty, p->mx, p->my, p->n);

	auto ad = reinterpret_cast<appdata*>(data);

	if (ad->event.shoving) {
		Tangram::handleShoveGesture(p->y2 - ad->event.last_y);

	} else if (ad->event.panning) {
		Tangram::handlePanGesture(ad->event.last_x, ad->event.last_y, p->x2,
				p->y2);
	} else {
		double dx = (ad->event.last_x - p->x2);
		double dy = (ad->event.last_y - p->y2);
		if (dx * dx + dy * dy > 50 * 50) {
			LOGD("START PANNING");
			ad->event.panning = true;
		} else {
			return EVAS_EVENT_FLAG_ON_HOLD;
		}
	}

	ad->event.last_x = p->x2;
	ad->event.last_y = p->y2;

	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags momentum_end(void *data, void *event_info) {
	Elm_Gesture_Momentum_Info *p = (Elm_Gesture_Momentum_Info *) event_info;
	LOGD("momentum ended x1,y1=<%d,%d> x2,y2=<%d,%d> tx,ty=<%d,%d> mx=<%d> my=<%d> n=>%u>\n",
			p->x1, p->y1, p->x2, p->y2, p->tx, p->ty, p->mx, p->my, p->n);

	auto ad = reinterpret_cast<appdata*>(data);

	if (p->n == 1) {
		Tangram::handleFlingGesture(ad->event.last_x, ad->event.last_y, p->mx,
				p->my);
	}

	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags momentum_abort(void *data, void *event_info) {
//	LOGD( "momentum abort\n");
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags line_start(void *data, void *event_info) {
	Elm_Gesture_Line_Info *p = (Elm_Gesture_Line_Info *) event_info;
	LOGD("line started angle=<%lf> x1,y1=<%d,%d> x2,y2=<%d,%d> tx,ty=<%u,%u> n=<%u>\n",
			p->angle, p->momentum.x1, p->momentum.y1, p->momentum.x2,
			p->momentum.y2, p->momentum.tx, p->momentum.ty, p->momentum.n);

	//auto ad = reinterpret_cast<appdata*>(data);
	//ad->event.shoving = false;
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags line_move(void *data, void *event_info) {
	Elm_Gesture_Line_Info *p = (Elm_Gesture_Line_Info *) event_info;
	LOGD("line move angle=<%lf> x1,y1=<%d,%d> x2,y2=<%d,%d> tx,ty=<%u,%u> n=<%u>\n",
			p->angle, p->momentum.x1, p->momentum.y1, p->momentum.x2,
			p->momentum.y2, p->momentum.tx, p->momentum.ty, p->momentum.n);
	auto ad = reinterpret_cast<appdata*>(data);

	if (!ad->event.can_shove || ad->event.rotation || ad->event.zooming) {
		return EVAS_EVENT_FLAG_ON_HOLD;
	}
	if (!ad->event.shoving && p->momentum.n == 2) {
		if ((p->angle < 30 || p->angle > 330)
				|| (p->angle < 210 && p->angle > 150)) {
			ad->event.shoving = true;
			ad->event.can_shove = false;
			LOGD( "START SHOVE");
		}
	}
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags line_end(void *data, void *event_info) {
	Elm_Gesture_Line_Info *p = (Elm_Gesture_Line_Info *) event_info;
	LOGD("line end angle=<%lf> x1,y1=<%d,%d> x2,y2=<%d,%d> tx,ty=<%u,%u> n=<%u>,\n",
			p->angle, p->momentum.x1, p->momentum.y1, p->momentum.x2,
			p->momentum.y2, p->momentum.tx, p->momentum.ty, p->momentum.n);

	//auto ad = reinterpret_cast<appdata*>(data);
	//ad->event.shoving = false;
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags line_abort(void *data, void *event_info) {
	Elm_Gesture_Line_Info *p = (Elm_Gesture_Line_Info *) event_info;
	LOGD("line abort\n");

	//auto ad = reinterpret_cast<appdata*>(data);
	//ad->event.shoving = false;
	return EVAS_EVENT_FLAG_ON_HOLD;
}

Evas_Object* glview_create(Evas_Object *win) {
	Evas_Object *glview;

	glview = elm_glview_add(win);
	elm_win_resize_object_add(win, glview);
	elm_glview_mode_set(glview, static_cast<Elm_GLView_Mode>(ELM_GLVIEW_DEPTH | //ELM_GLVIEW_STENCIL |
			// ELM_GLVIEW_DIRECT | // ELM_GLVIEW_CLIENT_SIDE_ROTATION |
			0));

	elm_glview_resize_policy_set(glview, ELM_GLVIEW_RESIZE_POLICY_RECREATE);

	elm_glview_render_policy_set(glview, ELM_GLVIEW_RENDER_POLICY_ON_DEMAND);
	elm_glview_init_func_set(glview, _init_gl);
	//elm_glview_del_func_set(glview, _del_gl);
	elm_glview_render_func_set(glview, _draw_gl);
	elm_glview_resize_func_set(glview, _resize_gl);
	evas_object_show(glview);

	return glview;
}

static void win_delete_request_cb(void *data, Evas_Object *obj,
		void *event_info) {
	ui_app_exit();
}

static void win_back_cb(void *data, Evas_Object *obj, void *event_info) {
	appdata_s *ad = (appdata_s*) data;
	/* Let window go to hide state. */
	elm_win_lower(ad->win);
}

static void file_chooser_cb(app_control_h request, app_control_h reply,
		app_control_result_e result, void *user_data) {
	auto ad = reinterpret_cast<appdata_s*>(user_data);

	if (result == APP_CONTROL_RESULT_FAILED) {
		LOGD("app control failed");
		return;
	}
	if (result == APP_CONTROL_RESULT_CANCELED) {
		LOGD("app control canceled");
		return;
	}

	bool is_array;
	app_control_is_extra_data_array(reply, "path", &is_array);

	ad->path[0] = '\0';
	if (is_array) {
		int i, count = 0;
		char **paths = NULL;
		app_control_get_extra_data_array(reply, "path", &paths, &count);

		if (paths != NULL) {
			for (i = 0; i < count; i++) {
				LOGD("file selected : %s", paths[i]);
			}
			if (count > 0)
				strcpy(ad->path, paths[0]);
			for (i = 0; i < count; i++)
				free(paths[i]);
			free(paths);
		} else {
			LOGD("file select failed!");
		}
	} else {
		char *path = NULL;
		app_control_get_extra_data(reply, "path", &path);
		if (path != NULL)
			strcpy(ad->path, path);
		else
			LOGD("file select failed!");
		free(path);
	}
	LOGD("file selected : %s", ad->path);

	Tangram::loadScene(ad->path);
}

static void _ctxpopup_item_cb_theme(void *data, Evas_Object *obj,
		void *event_info) {
	auto ad = reinterpret_cast<appdata_s*>(data);
	app_control_h ac;
	app_control_create(&ac);
	app_control_set_operation(ac, APP_CONTROL_OPERATION_PICK);
	//app_control_set_mime(ac, "*/*");

	app_control_send_launch_request(ac, file_chooser_cb, ad);
	app_control_destroy(ac);
}
static void _ctxpopup_item_cb_infos(void *data, Evas_Object *obj,
		void *event_info) {
	Tangram::toggleDebugFlag(Tangram::DebugFlags::tangram_infos);
	elm_ctxpopup_dismiss(obj);
}
static void _ctxpopup_item_cb_grid(void *data, Evas_Object *obj,
		void *event_info) {
	Tangram::toggleDebugFlag(Tangram::DebugFlags::tile_bounds);
	elm_ctxpopup_dismiss(obj);
}
static void _ctxpopup_dismissed_cb(void *data, Evas_Object *obj,
		void *event_info) {
	dlog_print(DLOG_INFO, LOG_TAG, "Ctxpopup dismissed\n");
}

static void move_more_ctxpopup(Evas_Object *ctxpopup) {
	Evas_Object *win;
	Evas_Coord w, h;
	int pos = -1;

	/* We convince the top widget is a window */
	win = elm_object_top_widget_get(ctxpopup);
	elm_win_screen_size_get(win, NULL, NULL, &w, &h);
	pos = elm_win_rotation_get(win);

	switch (pos) {
	case 0:
	case 180:
		evas_object_move(ctxpopup, (w / 2), h);
		break;
	case 90:
		evas_object_move(ctxpopup, (h / 2), w);
		break;
	case 270:
		evas_object_move(ctxpopup, (h / 2), w);
		break;
	}
}

static void _create_popup(void *data, Evas_Object *obj, void *event_info) {
	Evas_Object *parent = obj;
	Evas_Object *popup = elm_ctxpopup_add(parent);

	elm_popup_orient_set(popup, ELM_POPUP_ORIENT_BOTTOM);

	elm_ctxpopup_auto_hide_disabled_set(popup, EINA_TRUE);

	elm_object_style_set(popup, "more/default");
	elm_ctxpopup_direction_priority_set(popup, ELM_CTXPOPUP_DIRECTION_UP,
			ELM_CTXPOPUP_DIRECTION_LEFT, ELM_CTXPOPUP_DIRECTION_DOWN,
			ELM_CTXPOPUP_DIRECTION_RIGHT);

	//Evas_Object *home_icon = elm_icon_add(popup);
	//elm_icon_standard_set(home_icon, "home");

	elm_ctxpopup_item_append(popup, "Frame Infos", NULL,
			_ctxpopup_item_cb_infos, NULL);
	elm_ctxpopup_item_append(popup, "Tile Bounds", NULL, _ctxpopup_item_cb_grid,
	NULL);
	elm_ctxpopup_item_append(popup, "Theme File", NULL, _ctxpopup_item_cb_theme,
			data);

	evas_object_smart_callback_add(popup, "dismissed", _ctxpopup_dismissed_cb,
	NULL);

	// Register the EFL extension callback for the Back key event
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK,
			eext_popup_back_cb, NULL);
	move_more_ctxpopup(popup);
	evas_object_show(popup);
}

static void create_base_gui(appdata_s *ad) {

	//elm_config_accel_preference_set("opengl:msaa_high");
	elm_config_accel_preference_set("opengl");

	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_autodel_set(ad->win, EINA_TRUE);

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win,
				(const int *) (&rots), 4);
	}

	evas_object_smart_callback_add(ad->win, "delete,request",
			win_delete_request_cb, NULL);

	//eext_object_event_callback_add(ad->win, EEXT_CALLBACK_BACK, win_back_cb,
	//		ad);

	/* Conformant */
	/* Create and initialize elm_conformant.
	 elm_conformant is mandatory for base gui to have proper size
	 when indicator or virtual keypad is visible. */
	ad->conform = elm_conformant_add(ad->win);
	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_TRANSPARENT);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND,
	EVAS_HINT_EXPAND);
	evas_object_color_set(ad->conform, 255, 255, 255, 255);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	Evas_Object *nf = NULL;
	nf = elm_naviframe_add(ad->conform);
	elm_object_content_set(ad->conform, nf);

	evas_object_size_hint_weight_set(nf, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	auto obj = glview_create(nf);

	Evas_Object* g = elm_gesture_layer_add(obj);

	elm_gesture_layer_attach(g, obj);

	elm_gesture_layer_cb_set(g, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_START,
			zoom_start, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_END,
			zoom_end, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_ABORT,
			zoom_abort, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_MOVE,
			zoom_move, ad);

	elm_gesture_layer_cb_set(g, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_START,
			rotate_start, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_END,
			rotate_end, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_ABORT,
			rotate_abort, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_MOVE,
			rotate_move, ad);

	elm_gesture_layer_cb_set(g, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_START,
			momentum_start, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_END,
			momentum_end, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_ABORT,
			momentum_abort, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_MOVE,
			momentum_move, ad);

	elm_gesture_layer_cb_set(g, ELM_GESTURE_N_LINES, ELM_GESTURE_STATE_START,
			line_start, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_N_LINES, ELM_GESTURE_STATE_MOVE,
			line_move, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_N_LINES, ELM_GESTURE_STATE_END,
			line_end, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_N_LINES, ELM_GESTURE_STATE_ABORT,
			line_abort, ad);

	//elm_gesture_layer_line_min_length_set(g, 1000);
	//elm_gesture_layer_line_distance_tolerance_set(g, 0);

	elm_gesture_layer_rotate_angular_tolerance_set(g, 5);
	elm_gesture_layer_zoom_distance_tolerance_set(g, 20);

	evas_object_show(g);

	ad->glview = obj;
	evas_object_size_hint_align_set(ad->glview, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(ad->glview, EVAS_HINT_EXPAND,
	EVAS_HINT_EXPAND);

	//elm_object_content_set(ad->conform, obj);
	//elm_object_focus_set(obj, EINA_TRUE);

	eext_object_event_callback_add(nf, EEXT_CALLBACK_MORE, _create_popup, ad);
	eext_object_event_callback_add(nf, EEXT_CALLBACK_BACK, win_back_cb,
			ad->win);

	elm_naviframe_item_simple_push(nf, ad->glview);
	evas_object_show(nf);

	/* Show window after base gui is set up */
	evas_object_show(ad->win);

}

static bool app_create(void *data) {
	appdata_s *ad = (appdata_s*) data;
	create_base_gui(ad);

	initUrlRequests();

	return true;
}

static void app_control(app_control_h app_control, void *data) {
	/* Handle the launch request. */
}

static void app_pause(void *data) {
	/* Take necessary actions when application becomes invisible. */
}

static void app_resume(void *data) {
	/* Take necessary actions when application becomes visible. */
}

static void app_terminate(void *data) {
	/* Release all resources. */
}

static void ui_app_lang_changed(app_event_info_h event_info, void *user_data) {
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE,
			&locale);
	elm_language_set(locale);
	free(locale);
	return;
}

static void ui_app_orient_changed(app_event_info_h event_info,
		void *user_data) {
	return;
}

static void ui_app_region_changed(app_event_info_h event_info,
		void *user_data) {
}

static void ui_app_low_battery(app_event_info_h event_info, void *user_data) {
}

static void ui_app_low_memory(app_event_info_h event_info, void *user_data) {
}

int main(int argc, char *argv[]) {

	appdata_s ad = { 0, };
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = { 0, };
	app_event_handler_h handlers[5] = { NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY],
			APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY],
			APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED],
			APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED],
			APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED],
			APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, TAG, "app_main() is failed. err = %d", ret);
	}

	return ret;
}
