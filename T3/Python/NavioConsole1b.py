import customtkinter as ctk
import tkinter as tk
from pathlib import Path
from PIL import Image, ImageTk


# ============================================================
# Basic appearance
# ============================================================

ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("dark-blue")


BG = "#252525"
PANEL = "#151515"
PANEL_2 = "#1b1b1b"
CARD = "#2b2b2b"
CARD_HOVER = "#333333"
SELECTED = "#666666"
TEXT = "#eeeeee"
TEXT_DIM = "#a0a0a0"
LINE = "#3b3b3b"
BLUE = "#4b8cff"
ORANGE = "#ff9418"
GREEN = "#55d98a"


# ============================================================
# UI font helper
# ============================================================

def ui_font(size=13, weight="normal"):
    return ctk.CTkFont(
        family="Microsoft YaHei",
        size=size,
        weight=weight
    )


# ============================================================
# Main window
# ============================================================

FONT_FAMILY = "Microsoft YaHei"
DEVICE_IMAGE_PATH = str(Path(__file__).resolve().with_name("NavioChron U1.png"))

class TourStyleUI(ctk.CTk):

    def __init__(self):
        super().__init__()

        self.title("NavioChron Console 1.0.0")
        self.geometry("1500x920")
        self.minsize(1180, 760)
        self.configure(fg_color=BG)

        self._build_top_bar()
        self._build_left_panel()
        self._build_right_panel()

    # --------------------------------------------------------
    # Top bar
    # --------------------------------------------------------

    def _build_top_bar(self):
        bar = ctk.CTkFrame(
            self,
            height=48,
            corner_radius=0,
            fg_color=BG
        )
        bar.pack(fill="x", side="top")
        bar.pack_propagate(False)

        title = ctk.CTkLabel(
            bar,
            text="NavioChron Console 1.0.0",
            font=ui_font(16, "bold"),
            text_color=TEXT
        )
        title.place(relx=0.5, rely=0.52, anchor="center")

        # Right side icons
        icons = ["♧", "⬡", "♧", "—", "×"]
        for i, symbol in enumerate(icons):
            label = ctk.CTkLabel(
                bar,
                text=symbol,
                font=ui_font(21),
                text_color=TEXT_DIM,
                width=42
            )
            label.pack(side="right", padx=2)

    # --------------------------------------------------------
    # Left side
    # --------------------------------------------------------

    def _build_left_panel(self):

        left = ctk.CTkFrame(
            self,
            width=450,
            corner_radius=12,
            fg_color=PANEL
        )
        left.pack(
            side="left",
            fill="y",
            padx=(10, 5),
            pady=(0, 10)
        )
        left.pack_propagate(False)

        self._build_profile_list(left)
        self._build_device_view(left)
        self._build_bottom_toggle(left)

    # --------------------------------------------------------
    # Profile list
    # --------------------------------------------------------

    def _build_profile_list(self, parent):

        frame = ctk.CTkFrame(
            parent,
            fg_color=PANEL,
            corner_radius=0
        )
        frame.pack(
            fill="x",
            padx=12,
            pady=(12, 8)
        )

        header = ctk.CTkFrame(
            frame,
            height=38,
            fg_color="transparent"
        )
        header.pack(fill="x")
        header.pack_propagate(False)

        ctk.CTkLabel(
            header,
            text="预设列表",
            font=ui_font(16, "bold"),
            text_color=TEXT
        ).pack(side="left")

        ctk.CTkLabel(
            header,
            text="?",
            width=22,
            height=22,
            corner_radius=11,
            fg_color="#444444",
            text_color=TEXT_DIM
        ).pack(side="left", padx=7)

        for symbol in ["↗", "↙", "+"]:
            ctk.CTkButton(
                header,
                text=symbol,
                width=30,
                height=28,
                corner_radius=5,
                fg_color="transparent",
                hover_color=CARD_HOVER,
                text_color=TEXT_DIM
            ).pack(side="right", padx=2)

        # Automatic switching
        auto = ctk.CTkFrame(
            frame,
            height=68,
            fg_color=CARD,
            corner_radius=8
        )
        auto.pack(fill="x", pady=(3, 8))
        auto.pack_propagate(False)

        ctk.CTkLabel(
            auto,
            text="自动切换预设",
            font=ui_font(14, "bold"),
            text_color=TEXT
        ).place(x=12, y=10)

        ctk.CTkLabel(
            auto,
            text="预设将按照所关联的应用程序自动切换。",
            font=ui_font(11),
            text_color=TEXT_DIM
        ).place(x=12, y=38)

        switch = ctk.CTkSwitch(
            auto,
            text="",
            width=42,
            height=22,
            progress_color=BLUE,
            button_color="#dddddd",
            button_hover_color="#ffffff"
        )
        switch.select()
        switch.place(relx=1, x=-16, rely=0.5, anchor="e")

        profiles = [
            ("Premiere 调色", "Pr"),
            ("Capture One", "1"),
            ("CLIP STUDIO PAINT", "S"),
            ("PaintTool SAI 1.2.5", "S"),
            ("PaintTool SAI 2.0", "S"),
            ("DaVinci Resolve", "DR"),
            ("剪映", "剪"),
        ]

        list_frame = ctk.CTkFrame(
            frame,
            fg_color=PANEL,
            corner_radius=0
        )
        list_frame.pack(fill="x")

        for i, (name, icon) in enumerate(profiles):
            selected = name == "剪映"

            row = ctk.CTkFrame(
                list_frame,
                height=46,
                corner_radius=7,
                fg_color=SELECTED if selected else "transparent"
            )
            row.pack(fill="x", pady=2)
            row.pack_propagate(False)

            ctk.CTkLabel(
                row,
                text="☰",
                width=30,
                font=ui_font(16),
                text_color=BLUE if selected else TEXT
            ).pack(side="left", padx=(8, 2))

            ctk.CTkLabel(
                row,
                text=name,
                font=ui_font(14),
                text_color=TEXT
            ).pack(side="left")

            icon_box = ctk.CTkLabel(
                row,
                text=icon,
                width=30,
                height=30,
                corner_radius=5,
                fg_color="#eeeeee" if selected else "#303030",
                text_color="#111111" if selected else TEXT
            )
            icon_box.pack(side="right", padx=8)

    # --------------------------------------------------------
    # Device view
    # --------------------------------------------------------

    def _build_device_view(self, parent):

        outer = ctk.CTkFrame(
            parent,
            fg_color=PANEL_2,
            corner_radius=10
        )
        outer.pack(
            fill="both",
            expand=True,
            padx=12,
            pady=5
        )

        canvas = tk.Canvas(
            outer,
            bg=PANEL_2,
            highlightthickness=0
        )
        canvas.pack(fill="both", expand=True, padx=8, pady=8)

        self.device_canvas = canvas
        self.device_original = Image.open(DEVICE_IMAGE_PATH).convert("RGBA")
        self.device_photo = None
        self.device_image_id = None

        # Currently selected physical control.
        self.selected_control = None

        # Physical controls are defined in the ORIGINAL IMAGE coordinate
        # system (2048 x 974). They are converted to screen coordinates
        # automatically whenever the canvas is resized.
        #
        # Each entry:
        #   name      : internal control name
        #   label     : text shown to the user
        #   shape     : rectangle / oval
        #   coords    : original-image coordinates
        #
        # These regions deliberately follow the visible controls in the
        # supplied NavioChron image rather than using a generic grid.
        self.device_controls = [
            # Top three keys
            {
                "name": "top_left",
                "label": "上左键",
                "shape": "rect",
                "coords": (320, 68, 445, 193)
            },
            {
                "name": "top_middle",
                "label": "上中键",
                "shape": "rect",
                "coords": (445, 68, 628, 193)
            },
            {
                "name": "top_right",
                "label": "上右键",
                "shape": "rect",
                "coords": (628, 68, 750, 193)
            },

            # Left side wheel / side button
            {
                "name": "side_wheel",
                "label": "侧滚轮",
                "shape": "rect",
                "coords": (167, 158, 242, 319)
            },
            {
                "name": "side_button",
                "label": "侧键",
                "shape": "rect",
                "coords": (24, 194, 62, 286)
            },

            # Small center rotary knob
            {
                "name": "center_knob",
                "label": "旋钮",
                "shape": "oval",
                "coords": (435, 228, 640, 440)
            },

            # Center-left button
            {
                "name": "center_left",
                "label": "左功能键",
                "shape": "rect",
                "coords": (313, 358, 439, 487)
            },

            # Upper-right two buttons
            {
                "name": "upper_right_left",
                "label": "右上左键",
                "shape": "rect",
                "coords": (747, 226, 872, 349)
            },
            {
                "name": "upper_right_right",
                "label": "右上右键",
                "shape": "rect",
                "coords": (864, 287, 992, 413)
            },

            # Center direction / four-way controls
            {
                "name": "center_up",
                "label": "上键",
                "shape": "rect",
                "coords": (470, 472, 600, 599)
            },
            {
                "name": "center_left",
                "label": "左键",
                "shape": "rect",
                "coords": (352, 594, 477, 725)
            },
            {
                "name": "center_down",
                "label": "下键",
                "shape": "rect",
                "coords": (476, 594, 600, 725)
            },
            {
                "name": "center_right",
                "label": "右键",
                "shape": "rect",
                "coords": (597, 594, 722, 725)
            },

            # Large rotary wheel
            {
                "name": "large_wheel",
                "label": "转盘",
                "shape": "oval",
                "coords": (75, 472, 340, 740)
            },

            # Right vertical key + orange key
            {
                "name": "long_key",
                "label": "长键",
                "shape": "rect",
                "coords": (744, 472, 872, 724)
            },
            {
                "name": "short_key",
                "label": "短键",
                "shape": "rect",
                "coords": (864, 505, 991, 692)
            },
        ]

        self.device_hit_items = {}

        def image_to_canvas(x, y, w, h):
            iw, ih = self.device_original.size

            scale = min(
                (w - 35) / iw,
                (h - 55) / ih
            )

            nw = iw * scale
            nh = ih * scale

            offset_x = (w - nw) / 2
            offset_y = (h - nh) / 2 - 3

            return (
                offset_x + x * scale,
                offset_y + y * scale,
                scale
            )

        def redraw(event=None):
            canvas.delete("all")
            self.device_hit_items.clear()

            w = max(canvas.winfo_width(), 300)
            h = max(canvas.winfo_height(), 300)

            iw, ih = self.device_original.size

            scale = min(
                (w - 35) / iw,
                (h - 55) / ih
            )

            new_size = (
                max(1, int(iw * scale)),
                max(1, int(ih * scale))
            )

            resized = self.device_original.resize(
                new_size,
                Image.Resampling.LANCZOS
            )

            self.device_photo = ImageTk.PhotoImage(resized)

            nw, nh = new_size
            offset_x = (w - nw) / 2
            offset_y = (h - nh) / 2 - 3

            self.device_image_id = canvas.create_image(
                offset_x + nw / 2,
                offset_y + nh / 2,
                image=self.device_photo,
                anchor="center"
            )

            # Create transparent-ish clickable regions on top of the image.
            # We use stippled/outlined regions only when selected/hovered,
            # so the original device artwork remains clearly visible.
            for control in self.device_controls:
                x1, y1, x2, y2 = control["coords"]

                sx1 = offset_x + x1 * scale
                sy1 = offset_y + y1 * scale
                sx2 = offset_x + x2 * scale
                sy2 = offset_y + y2 * scale

                if control["shape"] == "oval":
                    item = canvas.create_oval(
                        sx1, sy1, sx2, sy2,
                        outline="",
                        width=2,
                        fill=""
                    )
                else:
                    item = canvas.create_rectangle(
                        sx1, sy1, sx2, sy2,
                        outline="",
                        width=2,
                        fill=""
                    )

                self.device_hit_items[item] = control["name"]

                canvas.tag_bind(
                    item,
                    "<Enter>",
                    lambda e, c=control: self._device_hover(c, True)
                )
                canvas.tag_bind(
                    item,
                    "<Leave>",
                    lambda e, c=control: self._device_hover(c, False)
                )
                canvas.tag_bind(
                    item,
                    "<Button-1>",
                    lambda e, c=control: self._device_click(c)
                )

            # Labels around the device.
            label_cfg = [
                (0.34, 0.05, "滚轮", "center"),
                (0.60, 0.05, "横键", "center"),
                (0.03, 0.39, "侧键", "w"),
                (0.03, 0.56, "NavioChron", "w"),
                (0.94, 0.43, "旋钮", "e"),
                (0.94, 0.56, "长键", "e"),
            ]

            for rx, ry, text_value, anchor in label_cfg:
                canvas.create_text(
                    w * rx,
                    h * ry,
                    text=text_value,
                    fill=TEXT_DIM,
                    font=(FONT_FAMILY, 10),
                    anchor=anchor
                )

            for rx, text_value in [
                (0.39, "上"),
                (0.47, "左"),
                (0.55, "下"),
                (0.63, "右"),
            ]:
                canvas.create_text(
                    w * rx,
                    h * 0.94,
                    text=text_value,
                    fill=TEXT_DIM,
                    font=(FONT_FAMILY, 10)
                )

            # Redraw selected/hover highlight after the image and hit areas.
            self._redraw_device_highlight(
                offset_x, offset_y, scale
            )

        canvas.bind("<Configure>", redraw)

        direction_btn = ctk.CTkButton(
            outer,
            text="方向",
            width=52,
            height=30,
            corner_radius=4,
            fg_color="#444444",
            hover_color="#555555",
            text_color=TEXT,
            font=ui_font(11)
        )
        direction_btn.place(
            relx=1,
            rely=1,
            x=-14,
            y=-12,
            anchor="se"
        )

        # Small status text. It will later become the selected control
        # configuration title / action editor.
        self.device_status = ctk.CTkLabel(
            outer,
            text="点击设备上的按键进行选择",
            text_color=TEXT_DIM,
            font=ui_font(11)
        )
        self.device_status.place(
            relx=0.5,
            rely=1,
            y=-14,
            anchor="s"
        )

    def _device_hover(self, control, entering):
        """
        Hover is intentionally lightweight. The selected control remains
        selected even when the mouse leaves it.
        """
        if entering:
            self.device_canvas.configure(cursor="hand2")
            self.device_status.configure(
                text=control["label"]
            )
        else:
            self.device_canvas.configure(cursor="arrow")
            if self.selected_control:
                self.device_status.configure(
                    text=f"已选择：{self.selected_control['label']}"
                )
            else:
                self.device_status.configure(
                    text="点击设备上的按键进行选择"
                )

    def _device_click(self, control):
        """
        Select one physical control. Later this callback can directly
        switch the right-side action editor to that control.
        """
        self.selected_control = control

        self.device_status.configure(
            text=f"已选择：{control['label']}"
        )

        self._refresh_device_highlight()

        print(
            f"Selected device control: "
            f"{control['name']} ({control['label']})"
        )

    def _refresh_device_highlight(self):
        """
        Force a redraw so the selected control is highlighted.
        """
        if hasattr(self, "device_canvas"):
            self.device_canvas.event_generate("<Configure>")

    def _redraw_device_highlight(self, offset_x, offset_y, scale):
        """
        Draw a visible orange highlight around the currently selected
        physical control.
        """
        canvas = self.device_canvas

        if not self.selected_control:
            return

        x1, y1, x2, y2 = self.selected_control["coords"]

        sx1 = offset_x + x1 * scale
        sy1 = offset_y + y1 * scale
        sx2 = offset_x + x2 * scale
        sy2 = offset_y + y2 * scale

        if self.selected_control["shape"] == "oval":
            canvas.create_oval(
                sx1, sy1, sx2, sy2,
                outline=ORANGE,
                width=3
            )
        else:
            canvas.create_rectangle(
                sx1, sy1, sx2, sy2,
                outline=ORANGE,
                width=3
            )

    # --------------------------------------------------------
    # Bottom toggle
    # --------------------------------------------------------

    def _build_bottom_toggle(self, parent):

        frame = ctk.CTkFrame(
            parent,
            height=52,
            fg_color=PANEL,
            corner_radius=8
        )
        frame.pack(fill="x", padx=12, pady=(4, 12))
        frame.pack_propagate(False)

        ctk.CTkLabel(
            frame,
            text="触觉反馈已允许",
            font=ui_font(13),
            text_color=TEXT
        ).pack(side="left", padx=12)

        sw = ctk.CTkSwitch(
            frame,
            text="",
            width=42,
            height=22,
            progress_color=BLUE
        )
        sw.select()
        sw.pack(side="right", padx=12)

    # --------------------------------------------------------
    # Right side
    # --------------------------------------------------------

    def _build_right_panel(self):

        outer = ctk.CTkFrame(
            self,
            corner_radius=12,
            fg_color=PANEL
        )
        outer.pack(
            side="left",
            fill="both",
            expand=True,
            padx=(5, 10),
            pady=(0, 10)
        )

        # Header
        header = ctk.CTkFrame(
            outer,
            height=58,
            fg_color="transparent"
        )
        header.pack(fill="x", padx=16, pady=(8, 0))
        header.pack_propagate(False)

        ctk.CTkLabel(
            header,
            text="预设配置",
            font=ui_font(17, "bold"),
            text_color=TEXT
        ).pack(side="left")

        ctk.CTkLabel(
            header,
            text="?",
            width=22,
            height=22,
            corner_radius=11,
            fg_color="#444444",
            text_color=TEXT_DIM
        ).pack(side="left", padx=7)

        ctk.CTkButton(
            header,
            text="重置",
            width=52,
            height=28,
            corner_radius=5,
            fg_color="#3b3b3b",
            hover_color="#4a4a4a"
        ).pack(side="left", padx=10)

        # HUD
        ctk.CTkLabel(
            header,
            text="HUD",
            font=ui_font(13),
            text_color=TEXT
        ).pack(side="right", padx=(10, 4))

        for txt, color in [("▦", "#888888"), ("⌘", "#55d98a")]:
            ctk.CTkButton(
                header,
                text=txt,
                width=44,
                height=30,
                corner_radius=5,
                fg_color="#333333",
                hover_color="#444444",
                text_color=color
            ).pack(side="right", padx=2)

        # Scroll area
        scroll = ctk.CTkScrollableFrame(
            outer,
            fg_color=PANEL,
            corner_radius=0,
            scrollbar_button_color="#444444",
            scrollbar_button_hover_color="#555555"
        )
        scroll.pack(fill="both", expand=True, padx=8, pady=(0, 8))

        self._section_title(scroll, "旋转区", ORANGE)
        self._action_card(
            scroll,
            "旋钮",
            "⟳",
            "[通用] 移动时间线指针↑↓：变速",
            "⚙"
        )
        self._action_card(
            scroll,
            "单击",
            "♧",
            "[V] 启用/停用片段",
            ""
        )

        self._action_card(
            scroll,
            "滚轮",
            "▥",
            "[通用] Ctrl + 鼠标滚轮(反向)",
            ""
        )
        self._action_card(
            scroll,
            "单击",
            "⇩",
            "全局预览缩放",
            ""
        )

        # Dual rotary card
        card = ctk.CTkFrame(
            scroll,
            fg_color=CARD,
            corner_radius=8
        )
        card.pack(fill="x", pady=6)

        row = ctk.CTkFrame(card, fg_color="transparent", height=55)
        row.pack(fill="x", padx=12)
        row.pack_propagate(False)

        ctk.CTkLabel(
            row,
            text="◉",
            font=ui_font(20),
            text_color=ORANGE,
            width=40
        ).pack(side="left")

        ctk.CTkLabel(
            row,
            text="转盘",
            font=ui_font(13, "bold")
        ).pack(side="left")

        ctk.CTkLabel(
            row,
            text="[ALT+鼠标滚轮上滚]",
            font=ui_font(12),
            text_color=TEXT
        ).pack(side="left", padx=35)

        ctk.CTkLabel(
            row,
            text="[ALT+鼠标滚轮下滚]",
            font=ui_font(12),
            text_color=TEXT
        ).pack(side="right")

        self._section_title(scroll, "控制区", BLUE)

        controls = [
            ("侧键", "未设置"),
            ("横键", "分割"),
            ("长键", "删除"),
            ("短键", "播放/暂停"),
        ]

        for name, action in controls:
            row = ctk.CTkFrame(
                scroll,
                height=48,
                fg_color=CARD,
                corner_radius=0
            )
            row.pack(fill="x", pady=1)
            row.pack_propagate(False)

            ctk.CTkLabel(
                row,
                text="▭",
                width=45,
                text_color=BLUE,
                font=ui_font(17)
            ).pack(side="left")

            ctk.CTkLabel(
                row,
                text=name,
                width=90,
                anchor="w",
                font=ui_font(13)
            ).pack(side="left")

            ctk.CTkLabel(
                row,
                text=action,
                anchor="w",
                text_color=TEXT_DIM if action == "未设置" else TEXT,
                font=ui_font(13)
            ).pack(side="left", fill="x", expand=True)

            if action != "未设置":
                ctk.CTkLabel(
                    row,
                    text="UP",
                    width=34,
                    height=25,
                    corner_radius=5,
                    fg_color="#222222",
                    text_color=TEXT
                ).pack(side="right", padx=12)

        self._section_title(scroll, "T区", GREEN)

        for name, action in [
            ("T键", "撤销"),
            ("T键 + 方向", "移动"),
        ]:
            row = ctk.CTkFrame(
                scroll,
                height=46,
                fg_color=CARD,
                corner_radius=0
            )
            row.pack(fill="x", pady=1)
            row.pack_propagate(False)

            ctk.CTkLabel(
                row,
                text="◇",
                width=45,
                text_color=GREEN,
                font=ui_font(17)
            ).pack(side="left")

            ctk.CTkLabel(
                row,
                text=name,
                width=110,
                anchor="w"
            ).pack(side="left")

            ctk.CTkLabel(
                row,
                text=action,
                anchor="w"
            ).pack(side="left")

    # --------------------------------------------------------
    # Section title
    # --------------------------------------------------------

    def _section_title(self, parent, title, color):

        frame = ctk.CTkFrame(
            parent,
            height=42,
            fg_color="transparent"
        )
        frame.pack(fill="x", pady=(10, 2))
        frame.pack_propagate(False)

        ctk.CTkLabel(
            frame,
            text="■",
            text_color=color,
            font=ui_font(12)
        ).pack(side="left", padx=(2, 8))

        ctk.CTkLabel(
            frame,
            text=title,
            text_color=TEXT,
            font=ui_font(14, "bold")
        ).pack(side="left")

        ctk.CTkLabel(
            frame,
            text="?",
            width=20,
            height=20,
            corner_radius=10,
            fg_color="#444444",
            text_color=TEXT_DIM
        ).pack(side="left", padx=7)

        separator = ctk.CTkFrame(
            frame,
            height=1,
            fg_color=LINE
        )
        separator.pack(
            side="bottom",
            fill="x",
            padx=2
        )

    # --------------------------------------------------------
    # Action card
    # --------------------------------------------------------

    def _action_card(self, parent, name, icon, action, extra):

        card = ctk.CTkFrame(
            parent,
            height=72,
            fg_color=CARD,
            corner_radius=7
        )
        card.pack(fill="x", pady=3)
        card.pack_propagate(False)

        ctk.CTkLabel(
            card,
            text=icon,
            width=42,
            text_color=ORANGE,
            font=ui_font(21)
        ).pack(side="left", padx=(8, 0))

        ctk.CTkLabel(
            card,
            text=name,
            width=70,
            anchor="w",
            font=ui_font(13, "bold")
        ).pack(side="left")

        ctk.CTkLabel(
            card,
            text=action,
            anchor="w",
            text_color=TEXT,
            font=ui_font(12)
        ).pack(side="left", fill="x", expand=True)

        if extra:
            ctk.CTkLabel(
                card,
                text=extra,
                width=30,
                text_color=TEXT_DIM
            ).pack(side="right", padx=8)


if __name__ == "__main__":
    app = TourStyleUI()
    app.mainloop()
