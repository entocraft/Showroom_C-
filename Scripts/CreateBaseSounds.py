"""
Fabrique les SONS DE BASE du showroom (petits sons synthetiques) dans /Game/Showroom/Sounds :

  S_ExperienceStart, S_ExperienceEnd, S_StepChange, S_Highlight,
  S_SpotActivate, S_SpotReached, S_UIClick, S_Ambience (boucle)

Ce sont des sons "provisoires" : pour les remplacer, importer tes propres sons puis les choisir dans
Project Settings > Game > Showroom > Sons (ou reimporter un fichier par-dessus le meme asset).

A lancer : Output Log -> Cmd -> py "Scripts/CreateBaseSounds.py"
Les sons deja presents ne sont PAS ecrases (mettre FORCE = True pour tout refaire).
"""

import math
import os
import random
import struct
import wave

import unreal

FORCE = False
DEST = "/Game/Showroom/Sounds"
SR = 44100
OUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "GeneratedAudio")
os.makedirs(OUT_DIR, exist_ok=True)
random.seed(7)


# ----------------------------------------------------------------------------
# Petite synthese
# ----------------------------------------------------------------------------

def silence(seconds):
    return [0.0] * int(seconds * SR)


def mix(dst, src, at=0.0, gain=1.0):
    start = int(at * SR)
    if len(dst) < start + len(src):
        dst.extend([0.0] * (start + len(src) - len(dst)))
    for i, v in enumerate(src):
        dst[start + i] += v * gain
    return dst


def bell(freq, seconds, decay=4.0):
    """Note de cloche douce : quelques harmoniques qui s'eteignent."""
    partials = [(1.0, 1.0), (2.0, 0.35), (3.01, 0.15), (4.2, 0.06)]
    out = []
    for n in range(int(seconds * SR)):
        t = n / SR
        attack = min(t / 0.005, 1.0)
        v = 0.0
        for k, (mult, amp) in enumerate(partials):
            v += amp * math.exp(-decay * (1 + k * 0.6) * t) * math.sin(2 * math.pi * freq * mult * t)
        out.append(v * attack)
    return out


def sweep(f0, f1, seconds, noise=0.0):
    """Glissement de frequence (whoosh) avec un peu de souffle."""
    out = []
    phase = 0.0
    low = 0.0
    for n in range(int(seconds * SR)):
        t = n / SR
        x = t / seconds
        freq = f0 * (f1 / f0) ** x
        phase += 2 * math.pi * freq / SR
        env = math.sin(math.pi * x) ** 2
        low += 0.08 * (random.uniform(-1, 1) - low)  # bruit adouci
        out.append(env * (0.8 * math.sin(phase) + noise * low * 4.0))
    return out


def normalize(samples, peak=0.8):
    m = max(abs(v) for v in samples) or 1.0
    return [v * peak / m for v in samples]


def fade_out(samples, seconds=0.05):
    n = int(seconds * SR)
    for i in range(min(n, len(samples))):
        samples[-1 - i] *= i / n
    return samples


def write_wav(name, samples):
    path = os.path.join(OUT_DIR, name + ".wav")
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1.0, min(1.0, v)) * 32767)) for v in samples))
    return path


# ----------------------------------------------------------------------------
# Les sons
# ----------------------------------------------------------------------------

C5, E5, G5, C6, E6, A6 = 523.25, 659.25, 783.99, 1046.5, 1318.5, 1760.0


def s_experience_start():
    out = silence(1.9)
    for i, f in enumerate([C5, E5, G5, C6]):
        mix(out, bell(f, 1.4, 3.0), at=0.12 * i, gain=0.8 if i < 3 else 1.0)
    return normalize(fade_out(out), 0.7)


def s_experience_end():
    out = silence(1.8)
    for i, f in enumerate([C6, G5, E5, C5]):
        mix(out, bell(f, 1.3, 3.5), at=0.13 * i)
    return normalize(fade_out(out), 0.6)


def s_step_change():
    out = silence(1.0)
    mix(out, bell(G5, 0.9, 5.0), 0.0)
    mix(out, bell(C6, 0.9, 5.0), 0.1)
    return normalize(fade_out(out), 0.55)


def s_highlight():
    """Petit scintillement doux : un objet est mis en valeur."""
    out = sweep(400, 1200, 0.35, noise=0.15)
    out = [v * 0.35 for v in out]
    mix(out, bell(G5, 1.0, 4.0), 0.05, 0.6)
    mix(out, bell(E6, 0.9, 4.5), 0.14, 0.45)
    return normalize(fade_out(out), 0.45)


def s_spot_activate():
    out = silence(1.6)
    shimmer = bell(E6, 1.4, 2.5)
    shimmer = [v * (0.75 + 0.25 * math.sin(2 * math.pi * 7 * n / SR)) for n, v in enumerate(shimmer)]
    mix(out, shimmer, 0.0, 0.6)
    mix(out, bell(A6, 1.2, 3.0), 0.08, 0.35)
    mix(out, bell(E5, 1.5, 2.0), 0.0, 0.5)
    return normalize(fade_out(out), 0.6)


def s_spot_reached():
    out = silence(1.5)
    for f in (C5, E5, G5):
        mix(out, bell(f, 1.3, 3.0), 0.0, 0.6)
    mix(out, bell(C6, 1.2, 3.0), 0.09, 0.8)
    return normalize(fade_out(out), 0.65)


def s_ui_click():
    out = []
    for n in range(int(0.045 * SR)):
        t = n / SR
        out.append(math.exp(-90 * t) * (math.sin(2 * math.pi * 2100 * t) + 0.4 * math.sin(2 * math.pi * 3300 * t)))
    return normalize(out, 0.4)


def s_ui_hover():
    """Petit "tic" tres doux quand le laser survole un bouton."""
    out = []
    for n in range(int(0.06 * SR)):
        t = n / SR
        out.append(math.exp(-70 * t) * math.sin(2 * math.pi * 1500 * t) * min(1.0, t / 0.004))
    return normalize(out, 0.22)


def s_ambience():
    """Nappe douce de 8 s qui boucle sans coupure (frequences multiples de 1/8 Hz)."""
    seconds = 8.0
    chord = [(110.0, 0.5), (165.0, 0.35), (220.0, 0.3), (277.5, 0.18), (330.0, 0.12)]
    out = []
    for n in range(int(seconds * SR)):
        t = n / SR
        lfo = 0.75 + 0.25 * math.sin(2 * math.pi * 0.25 * t)
        lfo2 = 0.7 + 0.3 * math.sin(2 * math.pi * 0.125 * t + 1.0)
        v = 0.0
        for i, (f, a) in enumerate(chord):
            v += a * (lfo if i % 2 == 0 else lfo2) * math.sin(2 * math.pi * f * t)
        out.append(v)
    return normalize(out, 0.45)


SOUNDS = {
    "S_ExperienceStart": s_experience_start,
    "S_ExperienceEnd": s_experience_end,
    "S_StepChange": s_step_change,
    "S_Highlight": s_highlight,
    "S_SpotActivate": s_spot_activate,
    "S_SpotReached": s_spot_reached,
    "S_UIClick": s_ui_click,
    "S_UIHover": s_ui_hover,
    "S_Ambience": s_ambience,
}

# ----------------------------------------------------------------------------
# Import dans le projet
# ----------------------------------------------------------------------------

# Sons des anciennes versions (les objets apparaissaient / disparaissaient) : plus utilises.
for old in ("S_ActorAppear", "S_ActorDisappear"):
    if unreal.EditorAssetLibrary.does_asset_exist(DEST + "/" + old):
        unreal.EditorAssetLibrary.delete_asset(DEST + "/" + old)
        unreal.log("[Sons] %s supprime (plus utilise)" % old)

tasks = []
for name, make in SOUNDS.items():
    if not FORCE and unreal.EditorAssetLibrary.does_asset_exist(DEST + "/" + name):
        unreal.log("[Sons] %s existe deja (garde)" % name)
        continue
    task = unreal.AssetImportTask()
    task.filename = write_wav(name, make())
    task.destination_path = DEST
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = True
    tasks.append(task)

if tasks:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

ambience = unreal.load_asset(DEST + "/S_Ambience")
if ambience and not ambience.get_editor_property("looping"):
    ambience.set_editor_property("looping", True)
    unreal.EditorAssetLibrary.save_loaded_asset(ambience)

for name in SOUNDS:
    unreal.log("[Sons] %s : %s" % (name, "OK" if unreal.EditorAssetLibrary.does_asset_exist(DEST + "/" + name) else "MANQUANT"))
