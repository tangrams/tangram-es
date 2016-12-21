#!/usr/bin/python3
import signal
import glfw
import sys

# export PYTHONPATH=../lib
if True:
    sys.path.append('../lib')
    import tangram


class Tangram:

    def __init__(self, scenefile):
        self.width = 800
        self.height = 600
        self.window = glfw.create_window(self.width, self.height, "Tangram", None, None)
        if not self.window:
            glfw.terminate()
            exit

        glfw.make_context_current(self.window)

        self.m = tangram.Map()
        self.m.load_scene(scenefile)
        self.m.setup_gl()

        self.panning = False
        self.last_x_down = 0
        self.last_y_down = 0

        glfw.set_mouse_button_callback(self.window, self.mouse_button_callback)
        glfw.set_cursor_pos_callback(self.window, self.cursor_pos_callback)
        glfw.set_key_callback(self.window, self.key_callback)
        glfw.set_scroll_callback(self.window, self.scroll_callback)
        glfw.set_window_size_callback(self.window,
                                      lambda win, w, h: self.m.resize(w, h))

        self.m.resize(self.width, self.height)
        self.running = False

    def feature_pick_callback(self, feature):
        if feature:
            print("name:", feature.properties.get_string("name"))
            self.m.log_screen("feature/name: " + feature.properties.get_string("name"))

    def label_pick_callback(self, feature):
        if feature:
            print("name:", feature.touch_item.properties.get_string("name"))
            self.m.log_screen("label/name: " + feature.touch_item.properties.get_string("name"))

    def mouse_button_callback(self, win, button, action, mods):
        if self.panning:
            self.panning = False

        if action == glfw.PRESS:
            self.m.pick_feature_at(self.last_x_down, self.last_y_down,
                                   self.feature_pick_callback)
            self.m.pick_label_at(self.last_x_down, self.last_y_down,
                                 self.label_pick_callback)

    def cursor_pos_callback(self, win, x, y):
        action = glfw.get_mouse_button(win, glfw.MOUSE_BUTTON_1)
        if action == glfw.PRESS:
            if self.panning:
                self.m.handle_pan_gesture(self.last_x_down, self.last_y_down, x, y)
            self.panning = True

        self.last_x_down = x
        self.last_y_down = y

    def scroll_callback(self, window, scrollx, scrolly):

        x, y = glfw.get_cursor_pos(window)

        rotating = (glfw.get_key(window, glfw.KEY_LEFT_ALT) == glfw.PRESS or
                    glfw.get_key(window, glfw.KEY_RIGHT_ALT) == glfw.PRESS)
        shoving = (glfw.get_key(window, glfw.KEY_LEFT_CONTROL) == glfw.PRESS or
                   glfw.get_key(window, glfw.KEY_RIGHT_CONTROL) == glfw.PRESS)

        scroll_span_multiplier = 0.05  # scaling for zoom and rotation
        scroll_distance_multiplier = 5.0  # scaling for shove

        if shoving:
            self.m.handle_shove_gesture(scroll_distance_multiplier * scrolly)
        elif rotating:
            self.m.handle_rotate_gesture(x, y, scroll_span_multiplier * scrolly)
        else:
            self.m.handle_pinch_gesture(x, y, 1.0 + scroll_span_multiplier * scrolly, 0)

    def key_callback(self, win, key, scancode, action, mods):
        if action == glfw.PRESS:
            if key == glfw.KEY_ESCAPE:
                glfw.set_window_should_close(self.window, True)
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

            # self.running = False

    def loop(self):
        self.running = True

        lastTime = glfw.get_time()

        while not glfw.window_should_close(self.window) and self.running:
            currentTime = glfw.get_time()
            delta = currentTime - lastTime
            lastTime = currentTime

            self.m.update(delta)
            self.m.render()

            glfw.swap_buffers(self.window)
            # glfw.poll_events()
            glfw.wait_events()



if not glfw.init():
    exit

# glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
# glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
# glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, True)
# glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)

def terminate():
    glfw.terminate()

def signal_handler(signal, frame):
    glfw.set_window_should_close(t.window, True)

signal.signal(signal.SIGINT, signal_handler)

if __name__ == "__main__":
    import argparse
    import os

    parser = argparse.ArgumentParser(description = '')
    parser.add_argument('-f', help = 'scene file.', dest = 'scene_path',
                        metavar = 'path', type = type(""),
                        default = os.getcwd() + '/scene.yaml')

    args = parser.parse_args()

    t = Tangram(args.scene_path)
    t.loop()
    terminate()
