"""
NARRATEUR : fabrique la voix de chaque etape d'experience par synthese vocale (TTS).

Pour chaque DetectionZone du niveau ouvert, et chaque etape :
  - le texte lu = "Narration Text" de l'etape, ou a defaut le titre puis la description,
  - la voix est creee dans /Game/Showroom/Narration/Auto et mise dans le champ "Narration" de l'etape.

Une etape dont le champ "Narration" contient un AUTRE son (un vrai enregistrement) n'est jamais touchee :
pour passer a une vraie voix, il suffit de glisser l'enregistrement dans "Narration".

A relancer apres avoir change des textes : Output Log -> Cmd -> py "Scripts/GenerateNarration.py"
Seules les voix dont le texte (ou le moteur) a change sont refaites.

MOTEURS (choisir ENGINE ci-dessous) :
  "piper"      : voix neuronale LOCALE et gratuite (par defaut). Installation : Scripts/InstallPiper.bat
  "windows"    : voix de Windows (Microsoft Hortense), plus robotique, rien a installer
  "elevenlabs" : service en ligne, voix tres humaines. Cle dans la variable d'environnement ELEVENLABS_API_KEY
  "azure"      : voix neuronales Microsoft en ligne. Variables AZURE_SPEECH_KEY et AZURE_SPEECH_REGION
Avec "elevenlabs" / "azure", les textes sont envoyes au service choisi.
"""

import hashlib
import json
import os
import subprocess
import urllib.request
import wave

import unreal

# ----------------------------------------------------------------------------
# Reglages
# ----------------------------------------------------------------------------

ENGINE = "piper"

PIPER_VOICE = "fr_FR-siwis-medium"    # voix installee par InstallPiper.bat
PIPER_SPEED = 1.08                    # > 1 = plus lent et pose, < 1 = plus rapide
PIPER_PAUSE = 0.35                    # silence entre deux phrases (s)

WINDOWS_RATE = 0                      # -10 (lent) a 10 (rapide)

ELEVENLABS_VOICE_ID = "21m00Tcm4TlvDq8ikWAM"   # a remplacer par l'id d'une voix francaise de ta bibliotheque
ELEVENLABS_MODEL = "eleven_multilingual_v2"

AZURE_VOICE = "fr-FR-DeniseNeural"    # ou fr-FR-HenriNeural, fr-FR-VivienneMultilingualNeural...

LEVEL = "/Game/XRFramework/Levels/L_XRTemplate"   # utilise si aucun niveau avec des zones n'est ouvert
DEST = "/Game/Showroom/Narration/Auto"

# ----------------------------------------------------------------------------

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
OUT_DIR = os.path.join(PROJECT, "Saved", "GeneratedAudio", "Narration")
TTS_DIR = os.path.join(PROJECT, "Saved", "TTS")
os.makedirs(OUT_DIR, exist_ok=True)
NO_WINDOW = getattr(subprocess, "CREATE_NO_WINDOW", 0)


def tts_piper(text, wav_path):
    python = os.path.join(TTS_DIR, "venv", "Scripts", "python.exe")
    model = os.path.join(TTS_DIR, "voices", PIPER_VOICE + ".onnx")
    if not (os.path.exists(python) and os.path.exists(model)):
        raise RuntimeError("Piper n'est pas installe : lancer Scripts/InstallPiper.bat (ou mettre ENGINE = \"windows\").")
    txt = wav_path[:-4] + ".txt"
    with open(txt, "w", encoding="utf-8") as f:
        f.write(text)
    result = subprocess.run([python, "-m", "piper", "-m", model, "-i", txt, "-f", wav_path,
                             "--length-scale", str(PIPER_SPEED), "--sentence-silence", str(PIPER_PAUSE)],
                            capture_output=True, text=True, creationflags=NO_WINDOW)
    if result.returncode != 0 or not os.path.exists(wav_path):
        raise RuntimeError("Piper : " + result.stderr[-500:])
    return "Piper " + PIPER_VOICE


def tts_windows(text, wav_path):
    ps1 = os.path.join(OUT_DIR, "tts_windows.ps1")
    with open(ps1, "w", encoding="utf-8-sig") as f:
        f.write(r"""
param([string]$TextFile, [string]$Out, [int]$Rate = 0)
Add-Type -AssemblyName System.Speech
$s = New-Object System.Speech.Synthesis.SpeechSynthesizer
$v = $s.GetInstalledVoices() | Where-Object { $_.Enabled -and $_.VoiceInfo.Culture.Name -like 'fr*' } | Select-Object -First 1
if ($v) { $s.SelectVoice($v.VoiceInfo.Name) }
$s.Rate = $Rate
$fmt = New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo(44100, [System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen, [System.Speech.AudioFormat.AudioChannel]::Mono)
$s.SetOutputToWaveFile($Out, $fmt)
$s.Speak([IO.File]::ReadAllText($TextFile, [Text.Encoding]::UTF8))
$s.Dispose()
if ($v) { Write-Output $v.VoiceInfo.Name } else { Write-Output 'voix Windows par defaut' }
""")
    txt = wav_path[:-4] + ".txt"
    with open(txt, "w", encoding="utf-8") as f:
        f.write(text)
    result = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", ps1,
                             "-TextFile", txt, "-Out", wav_path, "-Rate", str(WINDOWS_RATE)],
                            capture_output=True, text=True, creationflags=NO_WINDOW)
    if result.returncode != 0 or not os.path.exists(wav_path):
        raise RuntimeError("TTS Windows : " + result.stderr[-500:])
    return result.stdout.strip()


def write_pcm_wav(pcm, wav_path, rate):
    with wave.open(wav_path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(pcm)


def tts_elevenlabs(text, wav_path):
    key = os.environ.get("ELEVENLABS_API_KEY")
    if not key:
        raise RuntimeError("Variable d'environnement ELEVENLABS_API_KEY manquante.")
    url = "https://api.elevenlabs.io/v1/text-to-speech/%s?output_format=pcm_44100" % ELEVENLABS_VOICE_ID
    body = json.dumps({"text": text, "model_id": ELEVENLABS_MODEL}).encode("utf-8")
    req = urllib.request.Request(url, data=body, headers={"xi-api-key": key, "Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=120) as resp:
        write_pcm_wav(resp.read(), wav_path, 44100)
    return "ElevenLabs " + ELEVENLABS_VOICE_ID


def tts_azure(text, wav_path):
    key = os.environ.get("AZURE_SPEECH_KEY")
    region = os.environ.get("AZURE_SPEECH_REGION")
    if not key or not region:
        raise RuntimeError("Variables d'environnement AZURE_SPEECH_KEY / AZURE_SPEECH_REGION manquantes.")
    escaped = text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    ssml = ("<speak version='1.0' xml:lang='fr-FR'><voice name='%s'>%s</voice></speak>" % (AZURE_VOICE, escaped)).encode("utf-8")
    req = urllib.request.Request("https://%s.tts.speech.microsoft.com/cognitiveservices/v1" % region, data=ssml, headers={
        "Ocp-Apim-Subscription-Key": key,
        "Content-Type": "application/ssml+xml",
        "X-Microsoft-OutputFormat": "riff-44100hz-16bit-mono-pcm",
        "User-Agent": "ShowroomNarration",
    })
    with urllib.request.urlopen(req, timeout=120) as resp, open(wav_path, "wb") as f:
        f.write(resp.read())
    return "Azure " + AZURE_VOICE


ENGINES = {"piper": tts_piper, "windows": tts_windows, "elevenlabs": tts_elevenlabs, "azure": tts_azure}
ENGINE_ID = {"piper": "piper:%s:%s:%s" % (PIPER_VOICE, PIPER_SPEED, PIPER_PAUSE),
             "windows": "windows:%s" % WINDOWS_RATE,
             "elevenlabs": "elevenlabs:%s:%s" % (ELEVENLABS_VOICE_ID, ELEVENLABS_MODEL),
             "azure": "azure:%s" % AZURE_VOICE}[ENGINE]


def clean_name(label):
    return "".join(c if c.isalnum() else "_" for c in label)


# ----------------------------------------------------------------------------

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
zone_class = unreal.load_class(None, "/Script/Enit.DetectionZone")


def find_zones():
    return [a for a in actor_sub.get_all_level_actors() if a.get_class() == zone_class]


zones = find_zones()
if not zones:
    level_sub.load_level(LEVEL)
    zones = find_zones()

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
count = 0
for zone in zones:
    steps = zone.get_editor_property("steps")
    changed = False
    for i, step in enumerate(steps):
        current = step.get_editor_property("narration")
        if current and not current.get_path_name().startswith(DEST + "/"):
            unreal.log("[Narration] %s etape %d : enregistrement personnalise garde (%s)" % (zone.get_actor_label(), i + 1, current.get_name()))
            continue

        text = str(step.get_editor_property("narration_text")).strip()
        if not text:
            title = str(step.get_editor_property("title")).strip()
            desc = str(step.get_editor_property("description")).strip()
            text = ". ".join(p.rstrip(".") for p in (title, desc) if p) + "."
        text = " ".join(text.split())  # retours a la ligne -> espaces
        if text == ".":
            continue

        name = "VO_%s_%02d" % (clean_name(zone.get_actor_label()), i + 1)
        wav = os.path.join(OUT_DIR, name + ".wav")

        # Inutile de refaire une voix dont le texte et le moteur n'ont pas change.
        digest = hashlib.md5((text + "|" + ENGINE_ID).encode("utf-8")).hexdigest()
        stamp = wav + ".md5"
        if current and os.path.exists(stamp) and open(stamp).read() == digest:
            continue

        voice = ENGINES[ENGINE](text, wav)
        task = unreal.AssetImportTask()
        task.filename = wav
        task.destination_path = DEST
        task.destination_name = name
        task.replace_existing = True
        task.automated = True
        task.save = True
        asset_tools.import_asset_tasks([task])
        with open(stamp, "w") as f:
            f.write(digest)

        step.set_editor_property("narration", unreal.load_asset(DEST + "/" + name))
        steps[i] = step  # "step" est une copie : on la remet dans la liste
        changed = True
        count += 1
        unreal.log("[Narration] %s etape %d (%s) : \"%s\"" % (zone.get_actor_label(), i + 1, voice, text))

    if changed:
        zone.set_editor_property("steps", steps)

if count:
    level_sub.save_current_level()
unreal.log("[Narration] %d voix creees ou mises a jour (moteur : %s)." % (count, ENGINE))
