#!/usr/bin/python3

import signal
import time
import glfw
import sys

# export PYTHONPATH=../lib
if True:
    sys.path.append('../lib')
    import tangram

class BasicMap:
    def __init__(self, scene_path):
        tangram.init_globals()

        self.render_state = tangram.RenderState()
        self.view = tangram.View()
        self.input_handler = tangram.InputHandler(self.view)
        self.tile_worker = tangram.TileWorker(2)
        self.tile_manager = tangram.TileManager(self.tile_worker)
        self.scene = tangram.load_scene(scene_path)

        self.tile_manager.set_data_sources(self.scene.data_sources())
        self.tile_worker.set_scene(self.scene)

        self.marker_manager = tangram.MarkerManager()
        self.marker_manager.set_scene(self.scene)

        self.labels = tangram.Labels()

        self.view.set_max_pitch(60)

        # wait for downloads
        while self.scene.is_loading():
            print("dumdidum...")
            time.sleep(0.1)

    def __del__(self):
        self.teardown()

    def teardown(self):
        # static Primitives hold reference to
        # the instance RenderState...
        tangram.teardown_globals()

        # Necessary order of destruction!
        # Remove all references to Scene before disposing RenderState
        if self.tile_worker:
            self.tile_worker.stop()

        self.marker_manager = None
        self.tile_manager = None
        self.tile_worker = None
        self.scene = None

        if self.render_state:
            self.render_state.jobs_stop()
            self.render_state = None

    def resize(self, width, height):
        self.render_state.viewport(0, 0, width, height)
        self.view.set_size(width, height)

    def update(self, dt):
        self.input_handler.update(dt)
        self.view.update()

        styles = self.scene.styles()
        for style in styles:
            style.on_begin_update()

        view_state = self.view.state()
        self.tile_manager.update_tile_sets(view_state,
                                           self.view.get_visible_tiles())

        tiles = self.tile_manager.get_visible_tiles()

        if (self.view.changed_on_last_update() or
                self.tile_manager.has_tile_set_changed()):
            for tile in tiles:
                tile.update(dt, self.view)

            self.labels.update_label_set(view_state, dt, self.scene.style_set(), tiles,
                                         self.marker_manager.markers(),
                                         self.tile_manager.get_tile_cache())
        else:
            self.labels.update_labels(view_state, dt, self.scene.style_set(), tiles,
                                      self.marker_manager.markers())

    def render(self):

        self.render_state.cache_default_framebuffer()
        self.render_state.invalidate()

        self.render_state.jobs_run()

        tangram.frame_buffer_apply(self.render_state, self.view, self.scene)

        styles = self.scene.styles()
        tiles = self.tile_manager.get_visible_tiles()

        for style in styles:
            style.on_begin_frame(self.render_state)

        for style in styles:
            style.on_begin_draw_frame(self.render_state, self.view, self.scene)

            for tile in tiles:
                style.draw(self.render_state, tile)

            style.on_end_draw_frame()

        self.labels.draw_debug(self.render_state, self.view)

        tangram.frame_info_draw(self.render_state, self.view, self.tile_manager)


class MapView:

    def __init__(self, instance):
        self.m = instance
        self.width = 800
        self.height = 600

        glfw.window_hint(glfw.SAMPLES, 4)

        self.window = glfw.create_window(self.width, self.height, "Tangram", None, None)
        if not self.window:
            glfw.terminate()
            exit

        glfw.make_context_current(self.window)

        self.panning = False
        self.last_x_down = 0
        self.last_y_down = 0
        self.last_time_moved = 0
        self.last_x_velocity = 0
        self.last_y_velocity = 0

        glfw.set_mouse_button_callback(self.window, self.mouse_button_callback)
        glfw.set_cursor_pos_callback(self.window, self.cursor_pos_callback)
        glfw.set_key_callback(self.window, self.key_callback)
        glfw.set_scroll_callback(self.window, self.scroll_callback)
        glfw.set_window_size_callback(self.window,
                                      lambda win, w, h: self.m.resize(w, h))

        self.m.resize(self.width, self.height)
        self.running = False

    def __del__(self):
        self.teardown()

    def teardown(self):
        if self.m:
            self.m.teardown()
            self.m = None

        if self.window:
            glfw.destroy_window(self.window)
            self.window = None


    def mouse_button_callback(self, win, button, action, mods):
        if self.panning:
            self.panning = False
            x, y = glfw.get_cursor_pos(win)
            vx = min(max(self.last_x_velocity, -2000.0), 2000.0)
            vy = min(max(self.last_y_velocity, -2000.0), 2000.0)
            self.m.input_handler.handle_fling_gesture(x, y, vx, vy)

    def cursor_pos_callback(self, win, x, y):
        action = glfw.get_mouse_button(win, glfw.MOUSE_BUTTON_1)
        t = glfw.get_time()

        if action == glfw.PRESS:
            if self.panning:
                self.m.input_handler.handle_pan_gesture(self.last_x_down, self.last_y_down, x, y)

            self.panning = True
            self.last_x_velocity = (x - self.last_x_down) / (t - self.last_time_moved)
            self.last_y_velocity = (y - self.last_y_down) / (t - self.last_time_moved)
            self.last_x_down = x
            self.last_y_down = y

        self.last_time_moved = t


    def scroll_callback(self, window, scrollx, scrolly):
        rotating = (glfw.get_key(window, glfw.KEY_LEFT_ALT) == glfw.PRESS or
                    glfw.get_key(window, glfw.KEY_RIGHT_ALT) == glfw.PRESS)
        shoving = (glfw.get_key(window, glfw.KEY_LEFT_CONTROL) == glfw.PRESS or
                   glfw.get_key(window, glfw.KEY_RIGHT_CONTROL) == glfw.PRESS)

        scroll_span_multiplier = 0.05  # scaling for zoom and rotation
        scroll_distance_multiplier = 5.0  # scaling for shove

        x, y = glfw.get_cursor_pos(window)

        if shoving:
            self.m.input_handler.handle_shove_gesture(scroll_distance_multiplier * scrolly)
        elif rotating:
            self.m.input_handler.handle_rotate_gesture(x, y, scroll_span_multiplier * scrolly)
        else:
            self.m.input_handler.handle_pinch_gesture(x, y, 1.0 + scroll_span_multiplier * scrolly, 0)

    def key_callback(self, win, key, scancode, action, mods):
        if action == glfw.PRESS:
            if key == glfw.KEY_ESCAPE:
                print("shutting down...")
                self.running = False
            elif key == glfw.KEY_I:
                # jump to interpreter
                self.running = False
            elif key == glfw.KEY_1:
                tangram.toggle_debug_flag(tangram.FREEZE_TILES)
            elif key == glfw.KEY_2:
                tangram.toggle_debug_flag(tangram.PROXY_COLORS)
            elif key == glfw.KEY_3:
                tangram.toggle_debug_flag(tangram.TILE_BOUNDS)
            elif key == glfw.KEY_4:
                tangram.toggle_debug_flag(tangram.TILE_INFOS)
            elif key == glfw.KEY_5:
                tangram.toggle_debug_flag(tangram.LABELS)
            elif key == glfw.KEY_6:
                tangram.toggle_debug_flag(tangram.DRAW_ALL_LABELS)
            elif key == glfw.KEY_7:
                tangram.toggle_debug_flag(tangram.TANGRAM_INFOS)
            elif key == glfw.KEY_8:
                tangram.toggle_debug_flag(tangram.TANGRAM_STATS)
            elif key == glfw.KEY_9:
                tangram.toggle_debug_flag(tangram.SELECTION_BUFFER)


    def loop(self):
        self.running = True

        lastTime = glfw.get_time()

        while not glfw.window_should_close(self.window) and self.running:
            currentTime = glfw.get_time()
            delta = currentTime - lastTime
            lastTime = currentTime

            tangram.poke_network_queue()

            self.m.update(delta)
            self.m.render()

            glfw.swap_buffers(self.window)
            # glfw.poll_events()
            glfw.wait_events()


if __name__ == "__main__":
    import argparse
    import os

    if not glfw.init():
        exit

    parser = argparse.ArgumentParser(description = '')
    parser.add_argument('-f', help = 'scene file.', dest = 'scene_path',
                        metavar = 'path', type = type(""),
                        default = os.getcwd() + '/scene.yaml')

    args = parser.parse_args()

    t = MapView(BasicMap(args.scene_path))

    def signal_handler(signal, frame):
        t.running = False
    signal.signal(signal.SIGINT, signal_handler)

    try:
        t.loop()
    except Exception as e:
        print("ERROR: ", e)

    tangram.drain_network_queue()

    t.teardown()

    glfw.terminate()
