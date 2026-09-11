# Lab 9 — Simple Interactive 3D Scene (Unity)

A WASD-controllable player collects spinning cubes on a ground plane, with a
follow camera and an on-screen score. Verified end-to-end: project created,
scene built, and a standalone player built, run, and screenshotted — all
from the terminal via Unity's `-batchmode`/`-executeMethod`, no manual
GameObject placement in the Editor required.

## What's in this project

- `Assets/Scripts/PlayerController.cs` — WASD/arrow-key movement + jump via `CharacterController`
- `Assets/Scripts/CameraFollow.cs` — smooth third-person follow camera
- `Assets/Scripts/Collectible.cs` — spinning cube; destroyed and scores a point on player contact
- `Assets/Scripts/ScoreManager.cs` — updates an on-screen "Score: N" UI text
- `Assets/Scripts/ScreenshotCapture.cs` — auto-captures a screenshot 2s after the scene starts, to `Application.persistentDataPath`
- `Assets/Editor/Lab9SceneBuilder.cs` — builds the **entire scene from code** (ground plane, light, player, camera, 5 collectibles, score UI canvas) and saves it as `Assets/Scenes/Lab9Scene.unity`
- `Assets/Editor/Lab9Builder.cs` — builds a standalone macOS player of that scene into `Build/Lab9.app`

`Library/`, `Temp/`, `Build/`, `Logs/`, and `UserSettings/` are Unity-generated
and gitignored — they're recreated automatically.

## Opening in the Editor

Open Unity Hub → Projects → Add, and point it at this `LAB9` folder (the
project is already fully set up: `ProjectSettings/`, `Packages/manifest.json`
with `com.unity.ugui` added for the legacy UI Text component, etc.). Open
`Assets/Scenes/Lab9Scene.unity` and press **Play** — WASD to move, touch the
yellow cubes to collect them.

If you ever want to rebuild the scene from scratch, use the menu:
**Lab9 → Build Sample Scene**.

## Rebuilding and screenshotting from the terminal

This is exactly the process used to produce `screenshots/lab9_scene.png`:

```sh
UNITY_BIN="/Applications/Unity/Hub/Editor/6000.6.0f1/Unity.app/Contents/MacOS/Unity"
LAB9_PATH="/Users/aaditpani/Desktop/SEM7/CGVR/Lab01/LAB9"

# 1. (Re)build the scene from code
"$UNITY_BIN" -batchmode -quit -projectPath "$LAB9_PATH" -executeMethod Lab9SceneBuilder.BuildScene

# 2. Build a standalone player
"$UNITY_BIN" -batchmode -quit -projectPath "$LAB9_PATH" -executeMethod Lab9Builder.BuildStandalonePlayer

# 3. Run it — ScreenshotCapture.cs saves a PNG ~2s in, then keeps running until killed
"$LAB9_PATH/Build/Lab9.app/Contents/MacOS/LAB9" &
PID=$!
sleep 6
kill $PID

# 4. Copy the screenshot out of persistentDataPath into this repo
cp "$HOME/Library/Application Support/DefaultCompany/LAB9/lab9_scene.png" \
   "$LAB9_PATH/screenshots/lab9_scene.png"
```

## Notes

- `Packages/manifest.json` includes `com.unity.ugui` explicitly — a fresh
  `-createProject` doesn't add it automatically, and `UnityEngine.UI.Text`
  (used by `ScoreManager`) won't compile without it.
- The one-time Unity ID sign-in and license activation were done through
  Unity Hub's GUI (unavoidable — needs an account); everything after that
  (project creation, scene assembly, building, running, screenshotting) is
  scripted and repeatable from the CLI.
