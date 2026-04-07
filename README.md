# Paper Physics: Un-Stressed 📝✨

A 2D ragdoll physics simulator for **Nintendo DS** with a charming notebook doodle art style.

## 🎮 About

Interact with a stick-figure ragdoll inside a notebook! Grab, fling, pin, and draw your way through a physics sandbox. Use the DS touch screen and microphone to experiment with the laws of physics — or just have fun watching a little doodle bounce around.

## 🕹️ Controls

| Input | Action |
|-------|--------|
| **Touch & Drag** | Grab and move ragdoll parts |
| **Flick** | Launch the ragdoll with velocity |
| **A Button** | Pencil tool — draw physics shapes |
| **B Button** | Pin tool — fix parts to the screen |
| **X Button** | Rubber band tool |
| **Y Button** | Eraser tool |
| **L / R** | Flip gravity |
| **D-Pad Up/Down** | Switch game mode |
| **START** | Reset ragdoll |
| **SELECT** | Clear all objects |
| **Blow into mic** | Wind force! |

## 🎯 Game Modes

- **Sandbox** — All tools unlocked, no limits
- **Missions** — Complete physics challenges:
  - Keep the ragdoll airborne for 10 seconds
  - Hit a target in the corner
  - Build a structure to support the ragdoll

## 🏗️ Building

### Requirements
- [devkitPro](https://devkitpro.org/) with devkitARM and libnds

### Compile
```bash
make
```

The output `.nds` file can be run on real hardware (via flashcard) or in an emulator like [melonDS](https://melonds.kuribo64.net/) or [DeSmuME](https://desmume.org/).

### GitHub Actions
Every push automatically builds the ROM. Download the `.nds` from the Actions artifacts.

## 📐 Technical Details

- **Physics**: Custom Verlet integration engine with constraint solver
- **Rendering**: Software framebuffer rendering (notebook doodle style)
- **Math**: Fixed-point arithmetic (no floating point)
- **Target**: 60 FPS on Nintendo DS hardware

## 📜 License

This project is open source. Feel free to use, modify, and distribute.
