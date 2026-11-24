"""
ICONES des interfaces : dessinees par ce script (formes vectorielles -> PNG blanc anti-aliase),
puis importees dans /Game/Showroom/UI/Icons (T_Icon_<nom>). Blanches : le code les colore.

Pour en ajouter une : ecrire une fonction de forme dans ICONS (distance signee, voir les exemples),
relancer le script, puis l'utiliser en C++ avec ShowroomStyle::MakeIcon(..., TEXT("<nom>"), ...).
Pour la remplacer par ta propre image : importer un PNG blanc sur fond transparent sous le meme nom.

A lancer : Output Log -> Cmd -> py "Scripts/CreateIcons.py"
"""

import math
import os
import struct
import zlib

import unreal

SIZE = 128
DEST = "/Game/Showroom/UI/Icons"
OUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "GeneratedIcons")
os.makedirs(OUT_DIR, exist_ok=True)

# ----------------------------------------------------------------------------
# Formes (distance signee : < 0 a l'interieur). Coordonnees 0..1, y vers le bas.
# ----------------------------------------------------------------------------


def circle(px, py, cx, cy, r):
    return math.hypot(px - cx, py - cy) - r


def ring(px, py, cx, cy, r, w):
    return abs(circle(px, py, cx, cy, r)) - w / 2


def seg(px, py, ax, ay, bx, by, w):
    dx, dy = bx - ax, by - ay
    t = max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy)))
    return math.hypot(px - ax - dx * t, py - ay - dy * t) - w / 2


def box(px, py, cx, cy, hx, hy, r=0.0):
    qx, qy = abs(px - cx) - hx + r, abs(py - cy) - hy + r
    return math.hypot(max(qx, 0), max(qy, 0)) + min(max(qx, qy), 0) - r


def poly(px, py, pts):
    d = float("inf")
    inside = False
    n = len(pts)
    for i in range(n):
        ax, ay = pts[i]
        bx, by = pts[(i + 1) % n]
        d = min(d, seg(px, py, ax, ay, bx, by, 0.0))
        if (ay > py) != (by > py) and px < ax + (py - ay) * (bx - ax) / (by - ay):
            inside = not inside
    return -d if inside else d


def arc(px, py, cx, cy, r, w, a0, a1):
    """Arc de cercle de l'angle a0 a a1 (degres, sens horaire a l'ecran)."""
    a = math.degrees(math.atan2(py - cy, px - cx)) % 360
    span = (a1 - a0) % 360
    if (a - a0) % 360 <= span:
        return ring(px, py, cx, cy, r, w)
    ends = []
    for ang in (a0, a1):
        ex, ey = cx + r * math.cos(math.radians(ang)), cy + r * math.sin(math.radians(ang))
        ends.append(math.hypot(px - ex, py - ey) - w / 2)
    return min(ends)


def rot(px, py, cx, cy, deg):
    c, s = math.cos(math.radians(deg)), math.sin(math.radians(deg))
    x, y = px - cx, py - cy
    return cx + x * c + y * s, cy - x * s + y * c


U = min      # union


def sub(a, b):
    return max(a, -b)


W = 0.09     # epaisseur de trait


def i_grid(x, y):
    return U(box(x, y, .3, .3, .15, .15, .05), box(x, y, .7, .3, .15, .15, .05),
             box(x, y, .3, .7, .15, .15, .05), box(x, y, .7, .7, .15, .15, .05))


def i_info(x, y):
    return U(ring(x, y, .5, .5, .4, W), seg(x, y, .5, .47, .5, .72, W + .01), circle(x, y, .5, .31, .06))


def i_help(x, y):
    a = 405.0
    ex, ey = .5 + .13 * math.cos(math.radians(a)), .41 + .13 * math.sin(math.radians(a))
    return U(ring(x, y, .5, .5, .4, W), arc(x, y, .5, .41, .13, W, 180, a), seg(x, y, ex, ey, .5, .6, W),
             circle(x, y, .5, .74, .055))


def i_gear(x, y):
    d = circle(x, y, .5, .5, .29)
    for k in range(8):
        rx, ry = rot(x, y, .5, .5, k * 45)
        d = U(d, box(rx, ry, .5, .13, .075, .1, .03))
    return sub(d, circle(x, y, .5, .5, .12))


def i_pin(x, y):
    d = U(circle(x, y, .5, .4, .29), poly(x, y, [(.25, .5), (.75, .5), (.5, .93)]))
    return sub(d, circle(x, y, .5, .4, .11))


def i_clock(x, y):
    return U(ring(x, y, .5, .5, .4, W), seg(x, y, .5, .5, .5, .27, W), seg(x, y, .5, .5, .67, .6, W))


def i_tag(x, y):
    d = poly(x, y, [(.14, .17), (.56, .17), (.9, .5), (.56, .83), (.14, .83)]) - .03
    return sub(d, circle(x, y, .32, .5, .085))


def i_play(x, y):
    return poly(x, y, [(.32, .2), (.82, .5), (.32, .8)]) - .045


def i_stop(x, y):
    return box(x, y, .5, .5, .29, .29, .08)


def i_chevron_left(x, y):
    return U(seg(x, y, .62, .2, .36, .5, .12), seg(x, y, .36, .5, .62, .8, .12))


def i_chevron_right(x, y):
    return i_chevron_left(1 - x, y)


def i_check(x, y):
    return U(seg(x, y, .2, .53, .42, .75, .12), seg(x, y, .42, .75, .82, .28, .12))


def i_user(x, y):
    body = max(circle(x, y, .5, 1.0, .4), y - .9)
    return U(circle(x, y, .5, .32, .17), body)


def i_cube(x, y):
    p = [(.5, .12), (.85, .31), (.85, .69), (.5, .88), (.15, .69), (.15, .31)]
    d = min(seg(x, y, *p[i], *p[(i + 1) % 6], .075) for i in range(6))
    return U(d, seg(x, y, .15, .31, .5, .5, .075), seg(x, y, .85, .31, .5, .5, .075), seg(x, y, .5, .5, .5, .88, .075))


def i_close(x, y):
    return U(seg(x, y, .26, .26, .74, .74, .12), seg(x, y, .74, .26, .26, .74, .12))


def i_arrow_left(x, y):
    return U(seg(x, y, .22, .5, .82, .5, .11), seg(x, y, .46, .24, .2, .5, .11), seg(x, y, .2, .5, .46, .76, .11))


def i_sparkle(x, y):
    big = poly(x, y, [(.42, .1), (.52, .42), (.84, .52), (.52, .62), (.42, .94), (.32, .62), (0.0, .52), (.32, .42)])
    small = poly(x, y, [(.8, .06), (.84, .18), (.96, .22), (.84, .26), (.8, .38), (.76, .26), (.64, .22), (.76, .18)])
    return U(big, small)


def i_refresh(x, y):
    a = 320.0
    px, py = .5 + .3 * math.cos(math.radians(a)), .5 + .3 * math.sin(math.radians(a))
    nx, ny = math.cos(math.radians(a)), math.sin(math.radians(a))
    tx, ty = -ny, nx  # tangente (sens des angles croissants)
    head = poly(x, y, [(px + nx * .13, py + ny * .13), (px - nx * .13, py - ny * .13), (px + tx * .16, py + ty * .16)])
    return U(arc(x, y, .5, .5, .3, W, 40, a), head)


def i_power(x, y):
    return U(arc(x, y, .5, .54, .3, W, 300, 600), seg(x, y, .5, .12, .5, .48, W))


def i_pointer(x, y):
    return poly(x, y, [(.27, .12), (.27, .8), (.43, .65), (.55, .9), (.67, .85), (.55, .6), (.77, .6)]) - .02


def i_move(x, y):
    d = U(seg(x, y, .14, .5, .86, .5, W), seg(x, y, .5, .14, .5, .86, W))
    for (ax, ay, bx, by, cx, cy) in [(.26, .38, .14, .5, .26, .62), (.74, .38, .86, .5, .74, .62),
                                     (.38, .26, .5, .14, .62, .26), (.38, .74, .5, .86, .62, .74)]:
        d = U(d, seg(x, y, ax, ay, bx, by, W), seg(x, y, bx, by, cx, cy, W))
    return d


def i_steps(x, y):
    d = float("inf")
    for yy in (.28, .5, .72):
        d = U(d, circle(x, y, .22, yy, .065), seg(x, y, .4, yy, .82, yy, W))
    return d


def i_hand(x, y):
    # main ouverte stylisee : paume + 4 doigts + pouce
    d = box(x, y, .5, .66, .2, .2, .1)
    for fx, top in ((.34, .22), (.45, .14), (.56, .16), (.67, .26)):
        d = U(d, seg(x, y, fx, top, fx, .6, .1))
    return U(d, seg(x, y, .3, .66, .16, .48, .1))


def i_home(x, y):
    roof = poly(x, y, [(.5, .12), (.9, .5), (.1, .5)])
    house = box(x, y, .5, .66, .29, .24, .04)
    return sub(U(roof, house), box(x, y, .5, .78, .08, .14, .02))


ICONS = {
    "grid": i_grid, "info": i_info, "help": i_help, "gear": i_gear, "pin": i_pin, "clock": i_clock,
    "tag": i_tag, "play": i_play, "stop": i_stop, "chevron_left": i_chevron_left,
    "chevron_right": i_chevron_right, "check": i_check, "user": i_user, "cube": i_cube, "close": i_close,
    "arrow_left": i_arrow_left, "sparkle": i_sparkle, "refresh": i_refresh, "power": i_power,
    "pointer": i_pointer, "move": i_move, "steps": i_steps, "hand": i_hand, "home": i_home,
}

# ----------------------------------------------------------------------------
# Rendu PNG
# ----------------------------------------------------------------------------


def write_png(path, width, height, rgba_rows):
    raw = b"".join(b"\x00" + row for row in rgba_rows)
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def render_icon(fn):
    rows = []
    for j in range(SIZE):
        row = bytearray()
        for i in range(SIZE):
            d = fn((i + .5) / SIZE, (j + .5) / SIZE) * SIZE   # distance en pixels
            a = max(0.0, min(1.0, 0.5 - d))                    # bord anti-aliase
            row += bytes((255, 255, 255, int(a * 255 + .5)))
        rows.append(bytes(row))
    return rows


def render_shade():
    """Degrade vertical (transparent en haut -> noir en bas) pour rendre lisible un texte pose sur une image."""
    rows = []
    for j in range(128):
        t = j / 127.0
        a = int(255 * min(1.0, max(0.0, (t - 0.15) / 0.85)) ** 1.6 * 0.92)
        rows.append(bytes((0, 0, 0, a)) * 4)
    return rows


files = []
for name, fn in ICONS.items():
    path = os.path.join(OUT_DIR, "T_Icon_%s.png" % name)
    write_png(path, SIZE, SIZE, render_icon(fn))
    files.append(("T_Icon_" + name, path))
shade = os.path.join(OUT_DIR, "T_UI_ShadeBottom.png")
write_png(shade, 4, 128, render_shade())
files.append(("T_UI_ShadeBottom", shade))

tasks = []
for asset_name, path in files:
    t = unreal.AssetImportTask()
    t.filename = path
    t.destination_path = DEST
    t.destination_name = asset_name
    t.replace_existing = True
    t.automated = True
    t.save = False
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for asset_name, _ in files:
    tex = unreal.load_asset(DEST + "/" + asset_name)
    if not tex:
        unreal.log_error("[Icones] import rate : " + asset_name)
        continue
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(tex)
unreal.log("[Icones] %d icones dans %s" % (len(files), DEST))
