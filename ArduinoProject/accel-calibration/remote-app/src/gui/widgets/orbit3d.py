"""
A drawlist with a simple orthographic 3D projection; drag with the left button
to rotate it.

An orthographic view has no depth of its own: a point and its mirror behind the
centre land on the same pixel. So the view shades depth. Everything is drawn on one
of two layers, behind or in front of the plane through the centre facing the
viewer, and a translucent sphere sits between them: what is behind looks dimmed,
as through frosted glass. Points in front are filled dots, a little bigger the
closer they are; points behind are rings. Lines and circles are split where they
cross from one side to the other.

Layer: view.
"""

import dearpygui.dearpygui as dpg
import numpy as np

__all__ = ["DEFAULT_TINT", "DEFAULT_HIGHLIGHT", "Orbit3D"]

_DRAG_RAD_PER_PIXEL = 0.01
_MAX_PITCH_RAD = 1.5          # Just under 90 deg, so the view never turns upside down
_SCALE = 0.36                 # A unit vector takes this fraction of the smaller side
_CIRCLE_POINTS = 73
_FOG_COLOR = (24, 24, 28)    # The translucent unit sphere over what is behind it
_RIM = (150, 150, 160, 60)    # The sphere's outline
# Shading of the ball: discs that shrink towards a light up and to the left, each a
# little lighter, so they add up to a soft highlight
_SHADE_STEPS = 8
_MAX_SHADE_ALPHA = 8          # Alpha of each disc at highlight 1
DEFAULT_TINT = 0.30           # Opacity of the sphere over what is behind it, 0 to 1
DEFAULT_HIGHLIGHT = 0.2       # Strength of the highlight, 0 to 1
_LIGHT = np.array([-0.35, -0.4])   # Where the highlight is, in sphere radii (screen y down)
_DEPTH_SIZE = 0.25            # A point at depth +1 (nearest) is this much bigger, -1 smaller
_ARROW_HEAD = 7.0             # Arrow head size at depth 0, pixels
_DASH = 5.0                   # Dashes of a line behind the sphere: dash and gap, pixels


class Orbit3D:
    """
    A 3D view drawn on a drawlist, with depth shading.

    Attributes:
        tag: The drawlist.
    """

    def __init__(self, parent, width, height, yaw_rad=0.7, pitch_rad=0.35):
        """
        Args:
            parent: The item to add the drawlist to.
            width, height: Size of the view, pixels.
            yaw_rad, pitch_rad: The starting view angles.
        """
        self._width, self._height = width, height
        self._yaw, self._pitch = yaw_rad, pitch_rad
        self._last_mouse = None
        self.tag = dpg.add_drawlist(width=width, height=height, parent=parent)
        self._back = self._front = None
        self._tint, self._highlight, self._marks_behind = DEFAULT_TINT, DEFAULT_HIGHLIGHT, True
        self.clear()

    def set_shading(self, tint, highlight, marks_behind):
        """
        How depth is shown; takes effect at the next clear().

        Args:
            tint: Opacity of the sphere over what is behind it, 0 (none) to 1.
            highlight: Strength of the light spot on the ball, 0 (none) to 1.
            marks_behind: True to draw what is behind as rings, outlined arrow heads
                and dashes; False to draw it like what is in front.
        """
        self._tint = float(np.clip(tint, 0, 1))
        self._highlight = float(np.clip(highlight, 0, 1))
        self._marks_behind = bool(marks_behind)

    @property
    def size(self):
        """(width, height) of the view, pixels."""
        return self._width, self._height

    def resize(self, width, height):
        """
        Change the size of the view. The drawing is centred and scaled to the smaller
        side; redraw after a resize.

        Returns:
            True if the size changed.
        """
        width, height = int(width), int(height)
        if (width, height) == (self._width, self._height):
            return False
        self._width, self._height = width, height
        dpg.configure_item(self.tag, width=width, height=height)
        return True

    def handle_mouse(self):
        """Rotate while the left button drags on the view. Call every frame."""
        if dpg.is_item_hovered(self.tag) or self._last_mouse is not None:
            if dpg.is_mouse_button_down(dpg.mvMouseButton_Left):
                mouse = np.array(dpg.get_mouse_pos(local=False))
                if self._last_mouse is not None:
                    dx, dy = mouse - self._last_mouse
                    self._yaw -= dx * _DRAG_RAD_PER_PIXEL
                    self._pitch = float(np.clip(self._pitch + dy * _DRAG_RAD_PER_PIXEL,
                                                -_MAX_PITCH_RAD, _MAX_PITCH_RAD))
                self._last_mouse = mouse
                return
        self._last_mouse = None

    def clear(self):
        """Erase everything drawn, and set up the back layer, the sphere and the front layer."""
        dpg.delete_item(self.tag, children_only=True)
        self._back = dpg.add_draw_layer(parent=self.tag)
        center, radius = self._project(np.zeros(3))[0], self._scale()
        shade_alpha = round(_MAX_SHADE_ALPHA * self._highlight)
        with dpg.draw_layer(parent=self.tag):
            dpg.draw_circle(center, radius, color=_RIM,
                            fill=(*_FOG_COLOR, round(255 * self._tint)))
            for step in range(1, _SHADE_STEPS + 1 if shade_alpha else 1):
                k = step / (_SHADE_STEPS + 1)            # 0 at the rim to 1 at the light
                # Each disc stays inside the sphere: its centre moves towards the light
                # as it shrinks
                shade_center = np.asarray(center) + _LIGHT * k * radius
                dpg.draw_circle(list(shade_center), radius * (1 - k * 0.85),
                                color=(0, 0, 0, 0), fill=(255, 255, 255, shade_alpha))
        self._front = dpg.add_draw_layer(parent=self.tag)

    def line(self, a, b, color, thickness=1.0):
        """A line between two 3D points; each part on its side of the sphere."""
        for (start, _), (end, _), layer in self._split(np.asarray(a, float),
                                                       np.asarray(b, float)):
            dpg.draw_line(start, end, color=color, thickness=thickness, parent=layer)

    def arrow(self, start, end, color, thickness=2.0):
        """
        An arrow from start to end (3D points). Its colour carries a meaning (whether d
        will accept the position), so it is always drawn in front of the tinted sphere,
        never dimmed; depth is shown by its shape instead:
        - the part behind the sphere's centre is dashed;
        - the head is filled when the arrow points towards the viewer and an outline
          when it points away, and bigger when its tip is nearer;
        - seen end-on (too short on screen for a head) it is a dot (towards the
          viewer) or a ring (away).
        Without marks_behind (set_shading) it is always solid.
        """
        start, end = np.asarray(start, float), np.asarray(end, float)
        (start_xy, start_depth), (tip, depth) = self._project(start), self._project(end)
        is_towards = depth >= start_depth or not self._marks_behind
        size = _ARROW_HEAD * (1 + _DEPTH_SIZE * 2 * float(np.clip(depth, -1, 1)))
        layer = self._front
        fill = color if is_towards else (0, 0, 0, 0)
        tip = np.asarray(tip, float)
        on_screen = tip - np.asarray(start_xy, float)
        length = float(np.linalg.norm(on_screen))
        if length < 2 * size:                   # End-on: a dot or a ring at the tip
            dpg.draw_circle(list(tip), size * 0.8, color=color, thickness=thickness,
                            fill=fill, parent=layer)
            return
        # The shaft stops at the base of the head, so an outlined head stays empty
        direction = on_screen / length
        base = tip - direction * size * 1.6
        shaft_end = end - (end - start) * (size * 1.6 / length)
        for (a, _), (b, _), part_layer in self._split(start, shaft_end):
            if part_layer == self._back and self._marks_behind:
                _dashed_line(a, b, color, thickness, layer)
            else:
                dpg.draw_line(a, b, color=color, thickness=thickness, parent=layer)
        side = np.array([-direction[1], direction[0]]) * size
        dpg.draw_triangle(list(tip), list(base + side), list(base - side), color=color,
                          thickness=thickness, fill=fill, parent=layer)

    def text(self, point, text, color=(220, 220, 220), size=15):
        """Text beside a 3D point."""
        (x, y), depth = self._project(point)
        dpg.draw_text([x + 4, y - 8], text, color=color, size=size,
                      parent=self._layer(depth))

    def point(self, point, color, radius=3.5):
        """A dot at a 3D point: filled in front, a ring behind (unless marks_behind is
        off); bigger when nearer."""
        position, depth = self._project(point)
        radius *= 1 + _DEPTH_SIZE * float(np.clip(depth, -1, 1))
        is_filled = depth >= 0 or not self._marks_behind
        dpg.draw_circle(position, radius, color=color, thickness=1.5,
                        fill=color if is_filled else (0, 0, 0, 0), parent=self._layer(depth))

    def circle(self, normal_axis, color, thickness=1.0):
        """The unit circle around axis 0 (x), 1 (y) or 2 (z), its back half behind the sphere."""
        angles = np.linspace(0, 2 * np.pi, _CIRCLE_POINTS)
        c, s, zero = np.cos(angles), np.sin(angles), 0 * angles
        points = {0: np.c_[zero, c, s], 1: np.c_[c, zero, s], 2: np.c_[c, s, zero]}[normal_axis]
        projected = [self._project(p) for p in points]
        # Runs of segments on the same side, each drawn as one polyline
        run, run_layer = [projected[0][0]], None
        for (a, depth_a), (b, depth_b) in zip(projected, projected[1:]):
            layer = self._layer((depth_a + depth_b) / 2)
            if layer != run_layer and len(run) > 1:
                dpg.draw_polyline(run, color=color, thickness=thickness, parent=run_layer)
                run = [a]
            run_layer = layer
            run.append(b)
        dpg.draw_polyline(run, color=color, thickness=thickness, parent=run_layer)

    def axes(self, colors, line_alpha, length=1.15):
        """
        The x, y, z axes with their names, each line in its axis colour.

        Args:
            colors: (r, g, b) of x, y, z; the names are drawn in them.
            line_alpha: Opacity of the lines, 0 to 255: low, so they stay faint.
            length: Length of each axis, in units.
        """
        for i, name in enumerate("xyz"):
            end = np.eye(3)[i] * length
            self.line(-end * 0.15, end, (*colors[i][:3], line_alpha))
            self.text(end, name, color=colors[i])

    def _split(self, a, b):
        """
        The segment a-b cut where it crosses the plane through the centre facing the
        viewer.

        Returns:
            [((xy, depth), (xy, depth), layer), ...] from a to b: one or two parts.
        """
        pa, pb = self._project(a), self._project(b)
        depth_a, depth_b = pa[1], pb[1]
        if depth_a * depth_b >= 0:
            return [(pa, pb, self._layer((depth_a + depth_b) / 2))]
        cut = self._project(a + (b - a) * depth_a / (depth_a - depth_b))
        return [(pa, cut, self._layer(depth_a)), (cut, pb, self._layer(depth_b))]

    def _layer(self, depth):
        """The layer for a depth: in front of the sphere (>= 0) or behind it."""
        return self._front if depth >= 0 else self._back

    def _scale(self):
        """Pixels per unit length."""
        return _SCALE * min(self._width, self._height)

    def _project(self, point):
        """
        Screen position and depth of a 3D point: turn by yaw about z, tilt by pitch.

        Returns:
            ([x, y] in pixels, depth): depth > 0 is towards the viewer, in units.
        """
        x, y, z = point
        c, s = np.cos(self._yaw), np.sin(self._yaw)
        x1, y1 = c * x - s * y, s * x + c * y
        up = np.cos(self._pitch) * z + np.sin(self._pitch) * y1
        # Screen right x1 and up (0, sin, cos) in (x1, y1, z): their cross product points
        # at the viewer
        depth = -np.cos(self._pitch) * y1 + np.sin(self._pitch) * z
        scale = self._scale()
        return [self._width / 2 + scale * x1, self._height / 2 - scale * up], float(depth)


def _dashed_line(a, b, color, thickness, layer):
    """A dashed line between two screen points: dashes and gaps of _DASH pixels."""
    a, b = np.asarray(a, float), np.asarray(b, float)
    length = float(np.linalg.norm(b - a))
    if length == 0:
        return
    step = (b - a) / length
    for start in np.arange(0.0, length, 2 * _DASH):
        end = min(start + _DASH, length)
        dpg.draw_line(list(a + step * start), list(a + step * end), color=color,
                      thickness=thickness, parent=layer)
