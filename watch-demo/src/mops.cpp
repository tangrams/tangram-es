#include "mops.h"

#include <memory>
#include <network/net_connection.h>

#include "platform_tizen.h"

#include "tangram.h"
#include "log.h"

#define TAG "Tangram"

#define DEG_TO_RAD(x) (x) * (M_PI / 180.0)


using namespace Tangram;

struct App {
	Evas_Object *win = nullptr;
	Evas_Object *conform = nullptr;
	Evas_Object *glview = nullptr;

	struct {
		double angle = 0.0;
		double zoom = 0.0;
		Evas_Coord last_x = 0;
		Evas_Coord last_y = 0;
		bool rotation = false;
		bool zooming = false;
		bool can_shove = false;
		bool shoving = false;
		bool panning = false;
	} event;

	Tangram::Map *map = nullptr;
	std::shared_ptr<Tangram::TizenPlatform> platform;

	Ecore_Idle_Enterer *idler = nullptr;

	double time = -1;
	bool dirty = false;

	float zoom = -1;

	std::string resourcePath = "";
};



static void _draw_gl(Evas_Object *obj) {

	App* ad = (App*) evas_object_data_get(obj, "App");

	float t = 0;
	double time = ecore_time_get();
	if (ad->time > 0) {
		t = time - ad->time;
	}
	ad->time = time;

	//LOG(">>> DRAW GL %f", t);

	ad->map->update(t);
	//LOG("<<< RENDER");

	ad->map->render();

	//LOG("<<< DRAW GL");
}

static void _resize_gl(Evas_Object *obj) {
	LOG("RESIZE GL");

	int w, h;
	elm_glview_size_get(obj, &w, &h);
	if (w > 0 && h > 0) {
		App* ad = (App*) evas_object_data_get(obj, "App");
		ad->map->resize(w, h);
	}
}

static void _init_gl(Evas_Object *obj) {
	LOG("INIT GL");

	App* ad = (App*) evas_object_data_get(obj, "App");

	setEvasGlAPI(elm_glview_gl_api_get(obj));

	ad->map->setupGL();
}

static Evas_Event_Flags zoom_start(void *data, void *event_info) {
	Elm_Gesture_Zoom_Info *p = (Elm_Gesture_Zoom_Info *) event_info;
	LOGD("zoom started <%d,%d> zoom=<%f> radius=<%d> momentum=<%f>\n", p->x,
			p->y, p->zoom, p->radius, p->momentum);

	auto ad = reinterpret_cast<App*>(data);
	ad->event.zooming = false;
	ad->event.shoving = false;

	ad->zoom = -1;

	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags zoom_move(void *data, void *event_info) {
	Elm_Gesture_Zoom_Info *p = (Elm_Gesture_Zoom_Info *) event_info;
	auto ad = reinterpret_cast<App*>(data);

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

	ad->map->handlePinchGesture(p->x, p->y, 1 + (p->zoom - ad->event.zoom), 0);
	ad->event.zoom = p->zoom;

	elm_glview_changed_set(ad->glview);

	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags zoom_end(void *data, void *event_info) {
	Elm_Gesture_Zoom_Info *p = (Elm_Gesture_Zoom_Info *) event_info;
	LOGD("zoom end <%d,%d> zoom=<%f> radius=<%d> momentum=<%f>\n", p->x,
			p->y, p->zoom, p->radius, p->momentum);

	auto ad = reinterpret_cast<App*>(data);
	if (ad->event.zooming) {
		ad->event.zooming = false;

		ad->map->handlePinchGesture(p->x, p->y, 1 + (p->zoom - ad->event.zoom),
				p->momentum);

		elm_glview_changed_set(ad->glview);
		// stop panning to allow pinch animation to run
		ad->event.panning = false;
	}
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags zoom_abort(void *data, void *event_info EINA_UNUSED) {
	LOGD("zoom abort\n");
	auto ad = reinterpret_cast<App*>(data);
	ad->event.zooming = false;
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags rotate_start(void *data, void *event_info) {
	Elm_Gesture_Rotate_Info *p = (Elm_Gesture_Rotate_Info *) event_info;
	LOGD("rotate started <%d,%d> base=<%f> angle=<%f> radius=<%d> momentum=<%f>\n",
			p->x, p->y, p->base_angle, p->angle, p->radius, p->momentum);

	auto ad = reinterpret_cast<App*>(data);
	ad->event.rotation = false;
	ad->event.shoving = false;

	ad->event.can_shove = ((p->base_angle > 70 && p->base_angle < 110)
			|| (p->base_angle > 250 && p->base_angle < 290));

	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags rotate_abort(void *data, void *event_info EINA_UNUSED) {
	LOGD("rotate abort\n");
	auto ad = reinterpret_cast<App*>(data);
	ad->event.rotation = false;
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags rotate_move(void *data, void *event_info) {
	Elm_Gesture_Rotate_Info *p = (Elm_Gesture_Rotate_Info *) event_info;
	auto ad = reinterpret_cast<App*>(data);

	if (ad->event.shoving) {
		return EVAS_EVENT_FLAG_ON_HOLD;
	}

	if (!ad->event.rotation) {
		ad->event.rotation = true;
		ad->event.angle = p->angle;
		return EVAS_EVENT_FLAG_ON_HOLD;
	} LOGD("rotate move <%d,%d> base=<%f> angle=<%f> radius=<%d> momentum=<%f> => d:%f",
			p->x, p->y, p->base_angle, p->angle, p->radius, p->momentum,
			DEG_TO_RAD(p->angle - ad->event.angle));

	ad->map->handleRotateGesture(p->x, p->y,
			DEG_TO_RAD(p->angle - ad->event.angle));
	ad->event.angle = p->angle;

	elm_glview_changed_set(ad->glview);
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags rotate_end(void *data, void *event_info) {
	Elm_Gesture_Rotate_Info *p = (Elm_Gesture_Rotate_Info *) event_info;
	LOGD("rotate end <%d,%d> base=<%f> angle=<%f> radius=<%d> momentum=<%f>\n",
			p->x, p->y, p->base_angle, p->angle, p->radius, p->momentum);

	auto ad = reinterpret_cast<App*>(data);
	ad->event.rotation = false;
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags momentum_start(void *data, void *event_info) {
	Elm_Gesture_Momentum_Info *p = (Elm_Gesture_Momentum_Info *) event_info;
	LOGD("momentum started x1,y1=<%d,%d> tx,ty=<%d,%d> n=<%u>\n", p->x1,
			p->y1, p->tx, p->ty, p->n);

	auto ad = reinterpret_cast<App*>(data);
	ad->event.last_x = p->x1;
	ad->event.last_y = p->y1;

	ad->event.shoving = false;
	ad->event.can_shove = false;
	ad->event.panning = false;

	return EVAS_EVENT_FLAG_ON_HOLD;
}
static Evas_Event_Flags momentum_move(void *data, void *event_info) {
	Elm_Gesture_Momentum_Info *p = (Elm_Gesture_Momentum_Info *) event_info;
	//LOGD("momentum move x1,y1=<%d,%d> x2,y2=<%d,%d> tx,ty=<%d,%d> mx=<%d> my=<%d> n=<%u>\n",
	//		p->x1, p->y1, p->x2, p->y2, p->tx, p->ty, p->mx, p->my, p->n);

	auto ad = reinterpret_cast<App*>(data);

	if (ad->event.shoving) {
		ad->map->handleShoveGesture(p->y2 - ad->event.last_y);

	} else if (ad->event.panning) {
		ad->map->handlePanGesture(ad->event.last_x, ad->event.last_y, p->x2,
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

	auto ad = reinterpret_cast<App*>(data);

	if (p->n == 1) {
		ad->map->handleFlingGesture(ad->event.last_x, ad->event.last_y, p->mx,
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

	//auto ad = reinterpret_cast<App*>(data);
	//ad->event.shoving = false;
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags line_move(void *data, void *event_info) {
	Elm_Gesture_Line_Info *p = (Elm_Gesture_Line_Info *) event_info;
	LOGD("line move angle=<%lf> x1,y1=<%d,%d> x2,y2=<%d,%d> tx,ty=<%u,%u> n=<%u>\n",
			p->angle, p->momentum.x1, p->momentum.y1, p->momentum.x2,
			p->momentum.y2, p->momentum.tx, p->momentum.ty, p->momentum.n);
	auto ad = reinterpret_cast<App*>(data);

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

	//auto ad = reinterpret_cast<App*>(data);
	//ad->event.shoving = false;
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags line_abort(void *data, void *event_info) {
	Elm_Gesture_Line_Info *p = (Elm_Gesture_Line_Info *) event_info;
	LOGD("line abort\n");

	//auto ad = reinterpret_cast<App*>(data);
	//ad->event.shoving = false;
	return EVAS_EVENT_FLAG_ON_HOLD;
}

Evas_Object* glview_create(Evas_Object *win) {
	Evas_Object *glview;

	glview = elm_glview_add(win);
	elm_win_resize_object_add(win, glview);
	elm_glview_mode_set(glview,
			static_cast<Elm_GLView_Mode>(ELM_GLVIEW_DEPTH | ELM_GLVIEW_STENCIL
					| ELM_GLVIEW_DIRECT | // ELM_GLVIEW_CLIENT_SIDE_ROTATION |
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
	App *ad = (App*) data;
	/* Let window go to hide state. */
	elm_win_lower(ad->win);
}

static void create_base_gui(App *ad) {

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

	/* Conformant */
	/* Create and initialize elm_conformant.
	 elm_conformant is mandatory for base gui to have proper size
	 when indicator or virtual keypad is visible. */
	ad->conform = elm_conformant_add(ad->win);

	//Eext_Circle_Surface *sur = eext_circle_surface_conformant_add(ad->conform);

	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_HIDE);
	elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_TRANSPARENT);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND,	EVAS_HINT_EXPAND);
	evas_object_color_set(ad->conform, 255, 255, 255, 255);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	Evas_Object *nf = NULL;
	nf = elm_naviframe_add(ad->conform);
	elm_object_content_set(ad->conform, nf);

	evas_object_size_hint_weight_set(nf, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	Evas_Object *options = eext_more_option_add(nf);

	Eext_Object_Item *item  = eext_more_option_item_append(options);
	eext_more_option_item_part_text_set(item, "selector,main_text", "Default Scene");

	item  = eext_more_option_item_append(options);
    eext_more_option_item_part_text_set(item, "selector,main_text", "Bubble Wrap");

    item  = eext_more_option_item_append(options);
    eext_more_option_item_part_text_set(item, "selector,main_text", "Walkabout");


    evas_object_smart_callback_add(options, "item,clicked",
    		[](void *data, Evas_Object *obj, void *event_info){
    	auto item = static_cast<Eext_Object_Item*>(event_info);
    	auto ad = reinterpret_cast<App*>(data);

    	auto main_text = eext_more_option_item_part_text_get(item, "selector,main_text");

    	if (strcmp(main_text, "Default Scene") == 0) {
    		ad->map->loadSceneAsync((ad->resourcePath + "/scene.yaml").c_str());
    	} else if (strcmp(main_text, "Bubble Wrap") == 0) {
    		ad->map->loadSceneAsync((ad->resourcePath + "/bubble-wrap.yaml").c_str());
    	} else if (strcmp(main_text, "Walkabout") == 0) {
    		ad->map->loadSceneAsync((ad->resourcePath + "/walkabout-style.yaml").c_str());
    	}

    	eext_more_option_opened_set(obj, false);

    }, ad);

	//evas_object_size_hint_align_set(options, EVAS_HINT_FILL, EVAS_HINT_FILL);
	//evas_object_size_hint_weight_set(options, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_naviframe_item_push(nf, "More Option Demo", NULL, NULL, options, "empty");


	ad->glview = glview_create(nf);

	evas_object_data_set(ad->glview, "App", ad);

	Evas_Object* g = elm_gesture_layer_add(ad->glview);

	elm_gesture_layer_attach(g, ad->glview);

	elm_gesture_layer_cb_set(g, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_START, zoom_start, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_END, zoom_end, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_ABORT, zoom_abort, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_MOVE, zoom_move, ad);

	elm_gesture_layer_cb_set(g, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_START, rotate_start, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_END, rotate_end, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_ABORT, rotate_abort, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_MOVE,	rotate_move, ad);

	elm_gesture_layer_cb_set(g, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_START, momentum_start, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_END, momentum_end, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_ABORT, momentum_abort, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_MOVE, momentum_move, ad);

	elm_gesture_layer_cb_set(g, ELM_GESTURE_N_LINES, ELM_GESTURE_STATE_START, line_start, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_N_LINES, ELM_GESTURE_STATE_MOVE, line_move, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_N_LINES, ELM_GESTURE_STATE_END, line_end, ad);
	elm_gesture_layer_cb_set(g, ELM_GESTURE_N_LINES, ELM_GESTURE_STATE_ABORT, line_abort, ad);

	//elm_gesture_layer_line_min_length_set(g, 1000);
	//elm_gesture_layer_line_distance_tolerance_set(g, 0);

	elm_gesture_layer_rotate_angular_tolerance_set(g, 5);
	elm_gesture_layer_zoom_distance_tolerance_set(g, 20);

	evas_object_show(g);

	evas_object_size_hint_align_set(ad->glview, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(ad->glview, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	//elm_object_content_set(ad->conform, obj);
	//elm_object_focus_set(obj, EINA_TRUE);



	eext_rotary_object_event_callback_add(ad->glview,
			[](void *data, Evas_Object *obj, Eext_Rotary_Event_Info *info) {
				auto ad = reinterpret_cast<App*>(data);

				if (ad->zoom < 0) { ad->zoom = ad->map->getZoom();}

				if (info->direction == EEXT_ROTARY_DIRECTION_CLOCKWISE)	{
					ad->zoom += 0.3;
				} else {
					ad->zoom -= 0.3;
				}
				ad->map->setZoomEased(ad->zoom, 0.3f, EaseType::linear);

				elm_glview_changed_set(ad->glview);
				return EINA_TRUE;
			}, ad);


	evas_object_show(ad->glview);
	elm_object_part_content_set(options, "elm.swallow.content", ad->glview);

	//elm_naviframe_item_simple_push(nf, ad->glview);

	//evas_object_raise(options);


	evas_object_show(nf);

	/* Show window after base gui is set up */
	evas_object_show(ad->win);

	eext_object_event_callback_add(nf, EEXT_CALLBACK_BACK, win_back_cb, ad);

	// Activate rotary event for zooming
	eext_rotary_object_event_activated_set(ad->glview, EINA_TRUE);

	evas_object_smart_callback_add(options, "more,option,closed",
			[](void *data, Evas_Object *obj, void *event_info){
		auto ad = reinterpret_cast<App*>(data);
		eext_rotary_object_event_activated_set(ad->glview, EINA_TRUE);
	}, ad);
}


static bool get_proxy_address(char **proxyAddress)
{
	*proxyAddress = nullptr;

	connection_h con = nullptr;
	int errorCode = CONNECTION_ERROR_NOT_SUPPORTED;

	errorCode = connection_create(&con);

	if (errorCode == CONNECTION_ERROR_NONE) {
		errorCode = connection_get_proxy(con, CONNECTION_ADDRESS_FAMILY_IPV4, proxyAddress);
	}

	if (con)
		connection_destroy(con);

	return errorCode == CONNECTION_ERROR_NONE;
}

static bool app_create(void *data) {

	LOG(">>> APP_CREATE");
	App *ad = static_cast<App*>(data);

	Tangram::UrlClient::Options urlOptions;
	char* proxyAddress = nullptr;
	if (get_proxy_address(&proxyAddress) && proxyAddress) {
		urlOptions.proxyAddress = proxyAddress;
	}
	free(proxyAddress);

	ad->platform = std::make_shared<Tangram::TizenPlatform>(urlOptions);

	ad->map = new Tangram::Map(ad->platform);
	ad->map->setPixelScale(1.75f);

	char *app_res = app_get_resource_path();
	ad->resourcePath = app_res;
	free(app_res);

	ad->map->loadScene((ad->resourcePath + "/bubble-wrap.yaml").c_str(), true);

	create_base_gui(ad);

	ad->platform->setRenderCallbackFunction([ad](){
		ad->dirty = true;

		ecore_main_loop_thread_safe_call_async([](void* data){
		    // Doing elm_glview_changed_set here seems not reliably triggering
			// a redraw. Force to run the mainloop this way and set changed in
			// next idle_enterer.
			ecore_timer_add(0.0, [](void*){return EINA_FALSE;}, nullptr);
		}, ad->glview);
	});

	ad->idler = ecore_idle_enterer_add([](void* data){
		auto ad = reinterpret_cast<App*>(data);
		if (ad->dirty) {
			elm_glview_changed_set(ad->glview);
			ad->dirty = false;
		}
		return EINA_TRUE;
	}, ad);

	LOG("<<<< APP_CREATE");
	return true;
}

static void app_control(app_control_h app_control, void *data) {
	/* Handle the launch request. */
}

static void app_pause(void *data) {
	LOG(">>> PAUSE");
	/* Take necessary actions when application becomes invisible. */
}

static void app_resume(void *data) {
	LOG(">>> RESUME");
	/* Take necessary actions when application becomes visible. */
}

static void app_terminate(void *data) {
	LOG(">>> TERMINATE");
	/* Release all resources. */
	auto ad = (App*) data;

	if (ad->idler) {
		ecore_idle_enterer_del(ad->idler);
	}
	delete ad->map;
	LOG("<<< TERMINATE");
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

	App app;

	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = { 0, };
	app_event_handler_h handlers[5] = { NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY],
			APP_EVENT_LOW_BATTERY, ui_app_low_battery, &app);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY],
			APP_EVENT_LOW_MEMORY, ui_app_low_memory, &app);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED],
			APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &app);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED],
			APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &app);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED],
			APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &app);

	ret = ui_app_main(argc, argv, &event_callback, &app);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, TAG, "app_main() is failed. err = %d", ret);
	}
	LOG(">>> TERMINATED NICELY <<<");

	return ret;
}
