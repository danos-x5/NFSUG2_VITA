"""The Joypad picture's layout and leader rules, no rendering. Front End units: x from the screen centre, y in the
picture group's frame (PC label rows at 71..296)."""
import math

# ---- the Vita -------------------------------------------------------------------------------------------------------
ART_W, ART_H = 600, 270.62                  # vita_front_shaded.svg's size in art units
SCREEN = (118, 21.5, 363.5, 203)            # the Vita screen in art units: x, y, width, height
VITA_SCALE = 1.15                           # against the controls-overhaul picture
ART_UNIT = 0.40 * 233.6 / 256 * VITA_SCALE  # Front End units per art unit
VITA_X, VITA_Y = 0.0, 202.0                 # the Vita's centre
ART_X0 = VITA_X - ART_W / 2 * ART_UNIT
ART_Y0 = VITA_Y - 135.31 * ART_UNIT

# ---- the canvas and the label columns -------------------------------------------------------------------------------
CANVAS = (-424.0, 23.0, 424.0, 343.0)       # left, top, right, bottom: the full widescreen width
COLUMN = ART_W / 2 * ART_UNIT + 40          # label columns at -COLUMN (right aligned) and +COLUMN (left aligned)
LABEL_ROOM = 234.0                          # widest label: from the column to 400 units off the centre
GAP = 6.0                                   # from the label's edge to its leader
END_GAP = 4.5                               # from a leader's end to L, R (stopped short of the body's top edge)
STOP_GAP = 3.0                              # from a leader's end to a d-pad arm or face button outline
HMIN, DMIN = 10.0, 6.0                      # every leader: horizontal piece >= HMIN, then 45 degrees >= DMIN
LEADER_WIDTH = 1.6
EM = 15.5                                   # label font size the previews use (Helvetica Bold: "Brake / Reverse" 117)

# Label rows, top down: (slot, y). The approved layout of 24 Nov 2026, moved 20 down to clear the Customize row.
ROWS = {
    "L": [("l", 113), ("dpad_right", 138), ("dpad_up", 162), ("dpad_left", 196), ("dpad_down", 220),
          ("left_stick", 244), ("touch_career", 268), ("touch_map", 294)],
    "R": [("touch_shop", 65), ("touch_sms", 89), ("r", 113), ("square", 137), ("triangle", 161), ("circle", 196),
          ("cross", 220), ("right_stick", 244)],
}
# A leader whose horizontal piece ends where another's does, then 45 degrees, then vertical.
HORIZONTAL_ENDS = {"touch_career": "left_stick"}
DIAGONAL_CAP = {"l": 10.0, "r": 10.0}       # 45 degree piece capped, units

# Slot order of the generated header and the port (diagram.h).
SLOTS = ["triangle", "circle", "cross", "square", "l", "r", "dpad_up", "dpad_down", "dpad_left", "dpad_right",
         "left_stick", "right_stick", "touch_map", "touch_sms", "touch_shop", "touch_career"]

# ---- targets, in art units ------------------------------------------------------------------------------------------
# Button outlines: d-pad arms as rectangles (x0, y0, x1, y1), face buttons as circles (x, y, radius).
SHAPES = {
    "dpad_up": ("rect", (45.0, 60.0, 68.75, 84.0)), "dpad_down": ("rect", (45.0, 98.75, 68.75, 125.0)),
    "dpad_left": ("rect", (23.75, 81.0, 51.0, 97.5)), "dpad_right": ("rect", (63.75, 81.0, 91.0, 97.5)),
    "triangle": ("circle", (542.4, 66.2, 13.0)), "circle": ("circle", (569.0, 91.3, 13.0)),
    "square": ("circle", (515.9, 91.3, 13.0)), "cross": ("circle", (542.5, 117.0, 13.0)),
}
STICKS = {"left_stick": (73.6, 162.5), "right_stick": (526.5, 162.5)}
STICK_RADIUS = 25.0
SHOULDERS = {"l": (115.0, 2.0), "r": (485.0, 2.0)}     # on the body's top edge over the inner shoulder
# The free-roam HUD touch zones in Vita screen pixels (hud_touch.h); leaders end at their centroid.
TOUCH = {
    "touch_map": [(16, 331), (207, 331), (207, 529), (16, 529)],
    "touch_sms": [(96, 133), (109, 120), (199, 120), (199, 170), (109, 170), (96, 157)],
    "touch_shop": [(35, 40), (199, 40), (199, 112), (105, 112), (90, 127), (35, 127)],
    "touch_career": [(52, 177), (65, 164), (89, 164), (102, 177), (102, 267), (52, 267)],
}

# Sampling steps and tolerances of the approved render (1.6 texels a unit), kept so the layout comes out the same.
_T = 1 / 1.6
EPS = 0.01 * _T


def art(x, y):
    return ART_X0 + x * ART_UNIT, ART_Y0 + y * ART_UNIT


def centroid(poly):
    a = cx = cy = 0.0
    for (x0, y0), (x1, y1) in zip(poly, poly[1:] + poly[:1]):
        c = x0 * y1 - x1 * y0
        a += c
        cx += (x0 + x1) * c
        cy += (y0 + y1) * c
    return cx / (3 * a), cy / (3 * a)


def targets():
    """Slot -> (end point, how far short the leader stops)."""
    t = {}
    for name, (kind, v) in SHAPES.items():
        t[name] = (art((v[0] + v[2]) / 2, (v[1] + v[3]) / 2) if kind == "rect" else art(v[0], v[1]), 0.0)
    for name, p in STICKS.items():
        t[name] = (art(*p), STICK_RADIUS * ART_UNIT)
    for name, p in SHOULDERS.items():
        t[name] = (art(*p), END_GAP)
    sx, sy, sw, _ = SCREEN
    k = sw / 960.0
    for name, poly in TOUCH.items():
        x, y = centroid(poly)
        t[name] = (art(sx + x * k, sy + y * k), 0.0)
    return t


def shape_distance(name, pt):
    kind, v = SHAPES[name]
    x, y = (pt[0] - ART_X0) / ART_UNIT, (pt[1] - ART_Y0) / ART_UNIT
    if kind == "circle":
        d = math.hypot(x - v[0], y - v[1]) - v[2]
    else:
        d = math.hypot(max(v[0] - x, 0, x - v[2]), max(v[1] - y, 0, y - v[3]))
    return d * ART_UNIT


def over_shape(name, pts):
    """True when the line comes within STOP_GAP of the button."""
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        steps = max(1, int(math.hypot(x1 - x0, y1 - y0) / _T))
        for i in range(steps + 1):
            u = i / steps
            if shape_distance(name, (x0 + (x1 - x0) * u, y0 + (y1 - y0) * u)) < STOP_GAP:
                return True
    return False


def stop_short(name, pts):
    """Walks the end back until STOP_GAP clear of the button."""
    pts = list(pts)
    while len(pts) >= 2:
        (x0, y0), (x1, y1) = pts[-2], pts[-1]
        steps = max(1, int(math.hypot(x1 - x0, y1 - y0) / (0.25 * _T)))
        for i in range(steps + 1):
            u = 1 - i / steps
            q = (x0 + (x1 - x0) * u, y0 + (y1 - y0) * u)
            if shape_distance(name, q) >= STOP_GAP:
                return pts[:-1] + [q]
        pts = pts[:-1]
    return pts


def shorten(pts, by):
    """Stops the last piece short of the target by `by`."""
    if by <= 0 or len(pts) < 2:
        return pts
    (x0, y0), (x1, y1) = pts[-2], pts[-1]
    length = math.hypot(x1 - x0, y1 - y0)
    if length <= by:
        return pts[:-1]
    f = (length - by) / length
    return pts[:-1] + [(x0 + (x1 - x0) * f, y0 + (y1 - y0) * f)]


def rule_path(side, label_y, target, x_edge, diag=None):
    """Horizontal, then 45 degrees, then vertical when still needed."""
    cx, cy = target
    sign = 1 if side == "L" else -1
    dy = cy - label_y
    k = max(0.0, min(abs(dy), (cx - x_edge) * sign - HMIN))
    if diag is not None:
        k = min(k, diag)
    step = 1 if dy > 0 else -1
    pts = [(x_edge, label_y), (cx - sign * k, label_y)]
    if k > 0:
        pts.append((cx, label_y + step * k))
    if abs(dy) > k + EPS:
        pts.append((cx, cy))
    return pts


def follows_rule(p):
    if len(p) < 3:
        return False
    (x0, y0), (x1, y1), (x2, y2) = p[0], p[1], p[2]
    horizontal = abs(y1 - y0) < EPS and abs(x1 - x0) >= HMIN - EPS
    diagonal = abs(abs(x2 - x1) - abs(y2 - y1)) < 5 * EPS and abs(x2 - x1) >= DMIN - EPS
    return horizontal and diagonal


def label_x(side):
    return -COLUMN if side == "L" else COLUMN


def path(name, side, row, t):
    target, short = t[name]
    x_edge = label_x(side) + (GAP if side == "L" else -GAP)
    if name in SHAPES:
        full = abs(target[1] - row)
        options = [stop_short(name, rule_path(side, row, target, x_edge, diag=full - i * 0.5))
                   for i in range(int(full / 0.5) + 1)]

        def cost(p):
            return (0 if follows_rule(p) else 10) + sum(1 for o in SHAPES if o != name and over_shape(o, p))
        best = min(cost(p) for p in options)
        return next(p for p in options if cost(p) == best)
    return shorten(rule_path(side, row, target, x_edge, diag=DIAGONAL_CAP.get(name)), short)


def leaders():
    """Slot -> (side, label point, leader points)."""
    t = targets()
    out = {}
    for side, rows in ROWS.items():
        for name, y in rows:
            out[name] = (side, (label_x(side), float(y)), path(name, side, y, t))
    for name, other in HORIZONTAL_ENDS.items():
        side, label, p = out[name]
        (cx, cy), _ = t[name]
        x0, ly = p[0]
        hx = out[other][2][1][0]
        sign = 1 if side == "L" else -1
        dy = cy - ly
        k = min(abs(dy), abs(cx - hx))
        step = 1 if dy > 0 else -1
        q = [(x0, ly), (hx, ly), (hx + sign * k, ly + step * k)]
        if abs(dy) > k + EPS:
            q.append((hx + sign * k, cy))
        out[name] = (side, label, q)
    return out


def cross(a, b):
    (p1, p2), (p3, p4) = a, b

    def o(p, q, r):
        v = (q[0] - p[0]) * (r[1] - p[1]) - (q[1] - p[1]) * (r[0] - p[0])
        return 0 if abs(v) < 1e-9 else (1 if v > 0 else -1)
    return o(p1, p2, p3) * o(p1, p2, p4) < 0 and o(p3, p4, p1) * o(p3, p4, p2) < 0


def problems(lines=None):
    """Rule breaks: crossings, buttons passed over, bad shapes, shared rows."""
    lines = lines or leaders()
    out = []
    names = list(lines)
    for i, a in enumerate(names):
        for b in names[i + 1:]:
            pa, pb = lines[a][2], lines[b][2]
            if any(cross(s, r) for s in zip(pa, pa[1:]) for r in zip(pb, pb[1:])):
                out.append("%s crosses %s" % (a, b))
            if lines[a][0] == lines[b][0] and lines[a][1] == lines[b][1]:
                out.append("%s and %s share a label row" % (a, b))
    for name, (side, label, p) in lines.items():
        if not follows_rule(p) or any(abs(b[0] - a[0]) > EPS for a, b in zip(p[2:], p[3:])):
            out.append("%s breaks the leader rule" % name)
        for other in SHAPES:
            if other != name and over_shape(other, p):
                out.append("%s passes over %s" % (name, other))
        if any(not (CANVAS[0] <= x <= CANVAS[2] and CANVAS[1] <= y <= CANVAS[3]) for x, y in p):
            out.append("%s leaves the canvas" % name)
    if sorted(lines) != sorted(SLOTS):
        out.append("the rows do not hold every slot once")
    return out
