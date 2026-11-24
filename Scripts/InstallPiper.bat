@echo off
rem ---------------------------------------------------------------------------
rem Installe Piper (synthese vocale neuronale locale) pour le narrateur.
rem   - un environnement Python dans Saved\TTS\venv  (paquet piper-tts, ~30 Mo)
rem   - la voix francaise fr_FR-siwis-medium dans Saved\TTS\voices (~60 Mo, Hugging Face)
rem Il faut Python 3 installe (python.exe dans le PATH).
rem Ensuite : dans Unreal, Output Log -> Cmd -> py "Scripts/GenerateNarration.py"
rem Autres voix francaises : fr_FR-tom-medium, fr_FR-upmc-medium, fr_FR-gilles-low
rem   (changer VOICE ci-dessous ET PIPER_VOICE dans GenerateNarration.py)
rem ---------------------------------------------------------------------------
setlocal
set VOICE=fr_FR-siwis-medium
cd /d "%~dp0..\Saved"
if not exist TTS mkdir TTS
cd TTS

if not exist venv\Scripts\python.exe (
    echo Creation de l'environnement Python...
    python -m venv venv || goto :error
)
echo Installation de piper-tts...
venv\Scripts\python.exe -m pip install --disable-pip-version-check -q piper-tts || goto :error

if not exist voices mkdir voices
echo Telechargement de la voix %VOICE%...
venv\Scripts\python.exe -m piper.download_voices --download-dir voices %VOICE% || goto :error

echo.
echo Piper est pret.
pause
exit /b 0

:error
echo.
echo ERREUR pendant l'installation de Piper.
pause
exit /b 1
