import hid
import time
from pathlib import Path
from pynput.keyboard import Controller as Keyboard, Key
from pynput.mouse import Button
from pynput.mouse import Controller as Mouse
import pygetwindow
import json

keyboard = Keyboard()
mouse = Mouse()

VID = 0xcafe
PID = 0xbeef

# Keep profiles.json in the same directory as this script.
PROFILE_PATH = Path(__file__).resolve().with_name("profiles_u1.json")
PROFILE_RELOAD_INTERVAL = 2.0

KEY_MAP = {
    "ctrl": Key.ctrl,
    "shift": Key.shift,
    "alt": Key.alt,
    "space": Key.space,
    "backspace": Key.backspace,
    "enter": Key.enter,
    "tab": Key.tab,
    "esc": Key.esc,
    "up": Key.up,
    "down": Key.down,
    "left": Key.left,
    "right": Key.right,
    "mouse_left": Button.left,
    "mouse_middle": Button.middle,
    "mouse_right": Button.right,
    "mmup": "move_up",
    "mmdown": "move_down",
    "mmleft": "move_left",
    "mmright": "move_right"
}
MODIFIER_KEYS = {Key.ctrl, Key.shift, Key.alt, Key.cmd}

MOUSE_BUTTONS = {
    Button.left,
    Button.right,
    Button.middle,
}

MOUSE_ACTIONS = MOUSE_BUTTONS | {
    "move_up",
    "move_down",
    "move_left",
    "move_right",
    "scroll_up",
    "scroll_down",
    "scroll_left",
    "scroll_right",
}

# 映射表
profiles = {
    "premiere": {
        "title_contains": ["Adobe Premiere Pro"],
        "mapping": {
            0x02: ("key_combo", [Key.ctrl, 'b']),
            0x01: ("key", 'q'),
            0x03: ("key", 'w'),
            0x12: ("key", 'v'),
            0x13: ("key_combo", [Key.ctrl, 'z']),
            0x14: ("key_combo", [Key.ctrl, Key.shift, 'z']),
            0x24: ("key", Key.space),
            0x0f: ("alt_scroll", "down"),
            0x4f: ("alt_scroll", "up"),
            0x09: ("ctrl_scroll", "down"),
            0x49: ("ctrl_scroll", "up"),
        }
    },

    "photoshop": {
        "title_contains": ["Adobe Photoshop"],
        "mapping": {
            0x02: ("key_combo", [Key.ctrl, 'b']),
            0x01: ("key", 'q'),
            0x03: ("key", 'w'),
            0x12: ("key", 'v'),
            0x13: ("key_combo", [Key.ctrl, 'z']),
            0x14: ("key_combo", [Key.ctrl, Key.shift, 'z']),
            0x24: ("key", Key.space),
        }
    },

    "jianying": {
        "title_contains": ["剪映"],
        "mapping": {
            0x02: ("key_combo", [Key.ctrl, 'b']),
            0x00: ("key", Key.shift),
            0x01: ("key", 'q'),
            0x03: ("key", 'w'),
            0x12: ("key", 'v'),
            0x13: ("key_combo", [Key.ctrl, 'z']),
            0x14: ("key_combo", [Key.ctrl, Key.shift, 'z']),
            0x21: ("key", Key.alt),
            0x23: ("key", Key.backspace),
            0x24: ("key", Key.space),
            0x22: ("key", Key.up),
            0x31: ("key", Key.left),
            0x32: ("key", Key.down),
            0x33: ("key", Key.right),
            0x30: ("key", Key.ctrl),

            0x04: ("key", Key.left),
            0x44: ("key", Key.right),

            0x0f: ("alt_scroll", "down"),
            0x4f: ("alt_scroll", "up"),

            0x09: ("ctrl_scroll", "down"),
            0x49: ("ctrl_scroll", "up"),
        }
    },

    "default": {
        "title_contains": [],
        "mapping": {
            0x02: ("key_combo", [Key.ctrl, 'b']),
            0x00: ("key", Key.shift),
            0x01: ("key", 'q'),
            0x03: ("key", 'w'),
            0x12: ("key", 'v'),
            0x13: ("key_combo", [Key.ctrl, 'z']),
            0x14: ("key_combo", [Key.ctrl, Key.shift, 'z']),
            0x21: ("key", Key.alt),
            0x23: ("key", Key.backspace),
            0x24: ("key", Key.space),
            0x22: ("key", Key.up),
            0x31: ("key", Key.left),
            0x32: ("key", Key.down),
            0x33: ("key", Key.right),
            0x30: ("key", Key.ctrl),

            0x04: ("key", Key.left),
            0x44: ("key", Key.right),

            0x0f: ("alt_scroll", "down"),
            0x4f: ("alt_scroll", "up"),

            0x09: ("ctrl_scroll", "down"),
            0x49: ("ctrl_scroll", "up"),
        }
    }
}

ENCODER_CODES = {
    0x04: (0, "left"),   # 第一个编码器左转
    0x44: (0, "right"),  # 第一个编码器右转

    0x0F: (1, "left"),   # 第二个编码器左转
    0x4F: (1, "right"),  # 第二个编码器右转

    0x09: (2, "left"),   # 第三个编码器左转
    0x49: (2, "right"),  # 第三个编码器右转
}

encoder_states = {
    0: {"last_time": 0.0, "direction": None, "action": None},
    1: {"last_time": 0.0, "direction": None, "action": None},
    2: {"last_time": 0.0, "direction": None, "action": None},
}

pressed_actions = {}
profiles = {};
middle_drag_active = False
_last_profile_check = 0.0
_last_profile_mtime_ns = None

shift_middle_drag_active = False
last_encoder_time = 0
ENCODER_STOP_TIMEOUT = 0.25  # 250 ms

REPEAT_DELAY = 500
REPEAT_INTERVAL = 150
MOUSE_MOVE_STEP = 15

pressed_actions = {}


def normalize_key(key):
    if isinstance(key, str):
        return KEY_MAP.get(key.lower(), key)
    return key

def load_profiles(filename="profiles.json"):
    """Load JSON profiles and convert action names to pynput key objects."""
    with open(filename, "r", encoding="utf-8") as f:
        data = json.load(f)

    profiles = {}
    supported_actions = {
            "key", "key_combo", "alt_scroll", "ctrl_scroll",
            "mouse_middle_drag", "shift_mouse_middle_drag",
        }

    for profile_name, profile_data in data.items():

        mapping = {}

        for code, action in profile_data.get("mapping", {}).items():
            if not isinstance(action, list) or len(action) < 2:
                print(f"Warning: invalid action for {profile_name}/{code}")
                continue
            # JSON 中的 "0x02" -> Python 中的 0x02
            code = int(code, 16)

            action_type, argument = action[0], action[1]
            if action_type not in supported_actions:
                print(f"Warning: unsupported action {action_type!r} in {profile_name!r}")
                continue
            # Optional third tuple element enables hold-to-repeat.
            raw_repeat = (
                action[2]
                if len(action) >= 3 and isinstance(action[2], dict)
                else {}
            )
            try:
                delay_ms = max(0.0, float(raw_repeat.get("delay", REPEAT_DELAY)))
                interval_ms = max(1.0, float(raw_repeat.get("interval", REPEAT_INTERVAL)))
            except (TypeError, ValueError):
                delay_ms, interval_ms = REPEAT_DELAY, REPEAT_INTERVAL
            repeat_options = {
                "repeat": bool(raw_repeat.get("repeat", False)),
                "delay": delay_ms / 1000.0,
                "interval": interval_ms / 1000.0,
            }
            if action_type == "key":

                # key = action[1]

                # if isinstance(key, str) and key in KEY_MAP:
                #     key = KEY_MAP[key]
                argument = normalize_key(argument)

                mapping[code] = ("key", argument, repeat_options)

            elif action_type == "key_combo":

                # keys = []

                # for key in action[1]:

                #     if isinstance(key, str) and key in KEY_MAP:
                #         keys.append(KEY_MAP[key])
                #     else:
                #         keys.append(key)

                if not isinstance(argument, list):
                    print(f"Warning: key_combo argument must be a list: {profile_name}/{code}")
                    continue
                argument = [normalize_key(key) for key in argument]

                mapping[code] = ("key_combo", argument, repeat_options)

            elif action_type == "alt_scroll":

                mapping[code] = (
                    "alt_scroll",
                    action[1],
                    repeat_options
                )

            elif action_type == "ctrl_scroll":

                mapping[code] = (
                    "ctrl_scroll",
                    action[1],
                    repeat_options
                )
            elif action_type == "mouse_middle_drag":
            
                mapping[code] = (
                    "mouse_middle_drag",
                    action[1],
                    repeat_options
                )
            elif action_type == "shift_mouse_middle_drag":
                        
                mapping[code] = (
                    "shift_mouse_middle_drag",
                    action[1],
                    repeat_options
                )

        profiles[profile_name] = {
            "title_contains": profile_data["title_contains"],
            "mapping": mapping
        }

    if "default" not in profiles:
            raise ValueError('profiles.json must contain a "default" profile')
    return profiles

def reload_profiles_if_changed(force=False):
    """Check every 2 seconds; retain the last good config if the file is invalid."""
    global profiles, _last_profile_check, _last_profile_mtime_ns

    now = time.monotonic()
    if not force and now - _last_profile_check < PROFILE_RELOAD_INTERVAL:
        return
    _last_profile_check = now

    try:
        mtime_ns = PROFILE_PATH.stat().st_mtime_ns
        if not force and mtime_ns == _last_profile_mtime_ns:
            return

        # Swap in the new config only after the whole file loads successfully.
        new_profiles = load_profiles(PROFILE_PATH)
        profiles = new_profiles
        _last_profile_mtime_ns = mtime_ns
        print(f"Reloaded {PROFILE_PATH.name}")
    except (OSError, json.JSONDecodeError, KeyError, TypeError, ValueError) as e:
        print(f"Could not reload {PROFILE_PATH.name}: {e}")

def get_active_mapping():
    activewindow = pygetwindow.getActiveWindow()
    if activewindow is None:
        return profiles["default"]["mapping"]

    title = activewindow.title

    for name, profile in profiles.items():
        if name == "default":
            continue

        for keyword in profile["title_contains"]:
            if keyword.lower() in title.lower():
                return profile["mapping"]

    return profiles["default"]["mapping"]

def press_action(action):
    global shift_middle_drag_active, last_encoder_time, middle_drag_active
    t = action[0]

    if t == "key":
        keyboard.press(action[1])

    elif t == "key_combo":
        for k in action[1]:
            if k in MOUSE_BUTTONS:
                mouse.press(k)
            elif k == "move_up":
                mouse.move(0, -MOUSE_MOVE_STEP)
            elif k == "move_down":
                mouse.move(0, MOUSE_MOVE_STEP)
            elif k == "move_left":
                mouse.move(-MOUSE_MOVE_STEP, 0)
            elif k == "move_right":
                mouse.move(MOUSE_MOVE_STEP, 0)
            elif k == "scroll_up":
                mouse.scroll(0, 1)
            elif k == "scroll_down":
                mouse.scroll(0, -1)
            elif k == "scroll_left":
                mouse.scroll(-1, 0)
            elif k == "scroll_right":
                mouse.scroll(1, 0)
            else:
                keyboard.press(k)

    elif t == "ctrl_scroll":
        keyboard.press(Key.ctrl)
        if action[1] == "up":
            mouse.scroll(0, 1)
        else:
            mouse.scroll(0, -1)
        keyboard.release(Key.ctrl)

    elif t == "alt_scroll":
        keyboard.press(Key.alt)
        if action[1] == "up":
            mouse.scroll(0, 1)
        else:
            mouse.scroll(0, -1)
        keyboard.release(Key.alt)

    elif t == "mouse_middle_drag":
        if not middle_drag_active:
            mouse.press(Button.middle)
            middle_drag_active = True
        if action[1] == "mmup":
            mouse.move(0, -MOUSE_MOVE_STEP)
        elif action[1] == "mmdown":
            mouse.move(0, MOUSE_MOVE_STEP)
        elif action[1] == "mmleft":
            mouse.move(-MOUSE_MOVE_STEP, 0)
        elif action[1] == "mmright":
            mouse.move(MOUSE_MOVE_STEP, 0)
        last_encoder_time = time.monotonic()

    elif t == "shift_mouse_middle_drag":
        # 第一次转动时才按下按键
        if not shift_middle_drag_active:
            keyboard.press(Key.shift)
            mouse.press(Button.middle)
            shift_middle_drag_active = True
        # 每次编码器事件只移动鼠标，不释放按键
        if action[1] == "mmup":
            mouse.move(0, -MOUSE_MOVE_STEP)
        elif action[1] == "mmdown":
            mouse.move(0, MOUSE_MOVE_STEP)
        elif action[1] == "mmleft":
            mouse.move(-MOUSE_MOVE_STEP, 0)
        elif action[1] == "mmright":
            mouse.move(MOUSE_MOVE_STEP, 0)

        # 更新最后一次转动的时间
        last_encoder_time = time.monotonic()
        # mouse.release(Button.middle)
        # keyboard.release(Key.shift)


def release_action(action):
    global middle_drag_active, shift_middle_drag_active
    t = action[0]

    if t == "key":
        keyboard.release(action[1])

    elif t == "key_combo":
        for k in reversed(action[1]):
            if k in MOUSE_BUTTONS:
                mouse.release(k)
            elif k not in MOUSE_ACTIONS:
                keyboard.release(k)
    elif t == "mouse_middle_drag":
        mouse.release(Button.middle)
        middle_drag_active = False
    elif t == "shift_mouse_middle_drag":
        mouse.release(Button.middle)
        mouse.release(Key.shift)
        shift_middle_drag_active = False

def repeat_action(action):
    """Repeat the action effect while preserving any held drag buttons."""
    action_type, argument, _repeat_options = action

    if action_type == "key":
        # A modifier is a held state, not a useful repeated tap.
        if argument in MODIFIER_KEYS:
            return
        #keyboard.release(argument)
        keyboard.press(argument)
    elif action_type == "key_combo":
        # Release then re-press to generate another key-combination event.
        #for key in reversed(argument):
        #    keyboard.release(key)
        for k in argument:
            if k in MOUSE_BUTTONS:
                if (False):
                    mouse.press(k)
            elif k == "move_up":
                mouse.move(0, -MOUSE_MOVE_STEP)
            elif k == "move_down":
                mouse.move(0, MOUSE_MOVE_STEP)
            elif k == "move_left":
                mouse.move(-MOUSE_MOVE_STEP, 0)
            elif k == "move_right":
                mouse.move(MOUSE_MOVE_STEP, 0)
            elif k == "scroll_up":
                mouse.scroll(0, 1)
            elif k == "scroll_down":
                mouse.scroll(0, -1)
            elif k == "scroll_left":
                mouse.scroll(-1, 0)
            elif k == "scroll_right":
                mouse.scroll(1, 0)
            else:
                keyboard.press(k)
'''
def check_encoder_stop():
    global shift_middle_drag_active
    global middle_drag_active

    if shift_middle_drag_active:
        if time.monotonic() - last_encoder_time > ENCODER_STOP_TIMEOUT:
            mouse.release(Button.middle)
            keyboard.release(Key.shift)
            shift_middle_drag_active = False
    if middle_drag_active:
        if time.monotonic() - last_encoder_time > ENCODER_STOP_TIMEOUT:
            mouse.release(Button.middle)
            middle_drag_active = False
'''
            
def on_key_down(code, action):
    now = time.monotonic()

    if code in pressed_actions:
        return  # 忽略重复的按下事件

    press_action(action)

    pressed_actions[code] = {
        "action": action,
        "pressed_at": now,
        "last_repeat_at": now,
    }

def on_key_up(code):
    state = pressed_actions.pop(code, None)

    if state is not None:
        release_action(state["action"])
        print("Event:", hex(code), "release")
    else:
        print("Ignoring unmatched release:", hex(code))
        
def update_repeating_actions(now=None):
    if now is None:
        now = time.monotonic()

    for state in list(pressed_actions.values()):
        repeat_options = state["action"][2]

        if not repeat_options.get("repeat", False):
            continue

        if now - state["pressed_at"] < repeat_options["delay"]:
            continue
        if now - state["last_repeat_at"] < repeat_options["interval"]:
            continue
        #print("now - last_repeat_at: ", now, state["last_repeat_at"], now - state["last_repeat_at"])
        repeat_action(state["action"])
        state["last_repeat_at"] = now

def release_all_pressed_actions():
    """Best-effort cleanup after HID disconnect, avoiding stuck keys/buttons."""
    for state in list(pressed_actions.values()):
        try:
            release_action(state["action"])
        except Exception:
            pass
    pressed_actions.clear()

# 打开 HID
device = None

def connect():
    global device

    try:
        for d in hid.enumerate():
            if d['vendor_id'] == VID and d['product_id'] == PID:
                if d['usage_page'] == 0xFF00:

                    dev = hid.device()
                    dev.open_path(d['path'])

                    # Windows 必须放这里
                    dev.set_nonblocking(1)

                    print("Connected")
                    device = dev
                    return True

    except Exception as e:
        print("Connect error:", e)

    return False

def on_encoder_event(code, mapping):
    encoder_id, direction = ENCODER_CODES[code]
    action = mapping.get(code)

    if action is None:
        return

    state = encoder_states[encoder_id]
    now = time.monotonic()
    # 如果方向改变，先释放之前的操作
    if state["direction"] is not None and state["direction"] != direction:
        release_action(state["action"])

        state["direction"] = None
        state["action"] = None

    # 只有方向相同才延续原状态
    if state["direction"] is None:
        press_action(action)
        state["direction"] = direction
        state["action"] = action

    else:
        # 同方向连续旋转：
        # 对于滚轮等瞬时操作，每次事件都要执行。
        # 对于拖动操作，需要只移动鼠标，不重复按下鼠标键。
        repeat_action(state["action"])

    state["last_time"] = now

def check_encoder_stop():
    now = time.monotonic()

    for state in encoder_states.values():
        if state["direction"] is None:
            continue

        if now - state["last_time"] > ENCODER_STOP_TIMEOUT:
            release_action(state["action"])

            state["direction"] = None
            state["action"] = None
            state["last_time"] = 0.0
            
print("Vendor HID opened")

profiles = load_profiles()

while True:
    reload_profiles_if_changed()
    check_encoder_stop()
    update_repeating_actions()

    if device is None:
        connect()
        time.sleep(1)
        continue
    try:
        data = device.read(64)

        if data:
            code = data[1]

            release = False
            if code & 0x80:
                release = True
                code = code & 0x7F
            print("Event:", hex(data[0]), hex(data[1]), code)
            mapping = get_active_mapping()
            if code in ENCODER_CODES:
                if not release:
                    on_encoder_event(code, mapping)
            elif code in mapping:
                action = mapping[code]

                if release:
                    state = pressed_actions.pop(code, None)
                    if state is not None:
                        release_action(state["action"])
                        print("Event:", hex(code), "release")
                    else:
                        print("Ignoring unmatched release:", hex(code))

                    # if code in pressed_actions:
                    #     raction = pressed_actions[code]
                    #     release_action(raction)
                    #     del pressed_actions[code]
                    # else:
                    #     release_action(action)
                else:
                    if code not in pressed_actions:
                        if action is not None:
                            press_action(action)
                            now = time.monotonic()
                            pressed_actions[code] = {
                                "action": action,
                                "pressed_at": now,
                                "last_repeat_at": now,
                            }
                    else:
                        if action is not None:
                            press_action(action)

                print("Event:", hex(code), "release" if release else "press")
    except Exception as e:
        print("Disconnected:", e)
        release_all_pressed_actions()

        try:
            device.close()
        except:
            pass

        device = None

    time.sleep(0.001)
