# Crystal Verb

**A granular texture reverb and chaotic looper for creative sound design.**  
Built with [JUCE](https://juce.com/), it combines granular slicing, organic modulation, shimmer, formant filtering, tremolo, gate effects and a stunning 3D crystal visualizer.


---

## Features

- **Granular Looper** – Spawn up to 32 independent grains from a continuously recorded loop buffer. Each grain has its own pitch, speed, position, 3D spatial movement and organic window shape.
- **Organic Modulation** – Three uncorrelated LFOs create gentle pitch drift, density variation and timbral shifts, giving the sound a living, breathing quality.
- **Crystal Visualizer** – Real‑time 3D octahedron with dynamic particles, RMS halo, loop‑time arc and interactive rotation. Freeze mode tints the crystal blue.
- **Formant Focus Filter** – Dual band‑pass filter that shifts vowel‑like formants, morphing between “oooh” and “aaah” textures.
- **Shimmer & Reverb** – Built‑in Juce reverb with a pitch‑shifted shimmer feedback loop for ethereal tails.
- **Rhythmic Effects** – Square‑wave gate (Cutter) and sinusoidal tremolo, both synced to host BPM or manual rates.
- **Reverse Grains** – Probability‑based reverse playback with controllable amount.
- **Auto‑Ducking** – Compresses the wet signal when the input is loud.
- **BPM Snap** – Lock the loop time to musical divisions (1/8, 1/4, 1/2, 1 bar, 2 bars, 4 bars) using the host tempo or a manual BPM.
- **Freeze** – Hold the loop buffer indefinitely while grains continue to play.
- **8 Factory Presets** – Quickly explore the sonic palette: Jungle Thick, Ambient Drift, DnB Slicer, Pad Freeze, Glass Shimmer, Bass Loop, Glitch Storm and Init.
- **Dry/Wet** – Constant‑power blend for perfect mixing.

---

## Requirements

- **C++17** compatible compiler
- **JUCE** 7 or later (earlier versions may work with minor adjustments)
- **CMake** or the Projucer workflow

---

## Building

### Option 1: Projucer

1. Clone the repository:
   ```bash
   git clone https://github.com/your-username/crystal-verb.git
   cd crystal-verb
   ```
2. Open `CrystalVerb.jucer` (rename from `NewProject.jucer`) in the Projucer.
3. Set up your exporter (Xcode, Visual Studio, Linux Makefile).
4. Build the project.

### Option 2: CMake

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

> **Note:** The JUCE module folder must be reachable. If you cloned JUCE separately, adjust the CMake `juce_set_plugin_sources` paths accordingly.

---

## Parameters

| Parameter      | Range        | Description |
|----------------|--------------|-------------|
| **Loop Time**  | 0.1 s – 8.0 s (log) | Length of the loop buffer. SNAP to grid when BPM sync is active. |
| **Focus Freq** | 0 – 100%     | Formant filter morph – low values emphasise low vowels, high values brighten. |
| **Feedback**   | 0 – 90%      | Amount of loop buffer regeneration. |
| **Reverb**     | 0 – 100%     | Wet mix of the algorithmic reverb. |
| **Reverse**    | 0 – 100%     | Probability of grains playing backwards. |
| **Cutter**     | 0 – 100%     | Speed of the square‑wave gate LFO (0.5 Hz – 20 Hz). |
| **Tremolo**    | 0 – 100%     | Speed of the sinusoidal tremolo (0.1 Hz – 15 Hz). |
| **Pitch**      | ±12 semitones| Global transposition of all grains. |
| **Duck**       | 0 – 100%     | Auto‑ducking strength – wet signal attenuates when input level rises. |
| **BPM**        | 60 – 200     | Manual tempo (overridden by host tempo when DAW is playing). |
| **Wet/Dry**    | 0 – 100%     | Constant‑power dry/wet mix. |

### Toggle Buttons
- **ON** (Focus, Reverb, Reverse, Cutter, Tremolo) – Enable/disable each effect.
- **SNAP** – Activate BPM‑based loop time quantisation.
- **❄ FREEZE** – Freeze the loop buffer.

---

## Presets

Select from the **PRESET** dropdown in the top bar:

1. **Init** – Clean starting point.
2. **Jungle Thick** – Punchy short loop with reverse and heavy ducking.
3. **Ambient Drift** – Long, slowly evolving pads with heavy reverb and focus modulation.
4. **DnB Slicer** – Fast rhythmic gate, tight loop, snappy reverse.
5. **Pad Freeze** – Infinite frozen pad with shimmer.
6. **Glass Shimmer** – Bright, crystalline grains with shimmer and gentle tremolo.
7. **Bass Loop** – Sub‑octave focus, solid loop, no clutter.
8. **Glitch Storm** – Ultra‑short loop, extreme reverse and fast cutter.

---

## Visualizer

Click and drag on the 3D crystal to rotate it. Release to let it spin with inertia.  
The **loop‑time arc** rotates once per loop.  
Particles change colour based on the face of the octahedron they last touched.  
When **Freeze** is engaged, the crystal turns blue and a “* FROZEN” indicator appears.

---

## License

Crystal Verb is **dual‑licensed**.

- **Open Source** – The code in this repository is available under the [GNU General Public License v3.0](LICENSE). You are free to use, study, share and improve it, as long as you keep your modifications open under the same license.

- **Commercial** – If you wish to distribute Crystal Verb as part of a closed‑source commercial product, or you need a license without GPL copyleft restrictions, you may purchase a commercial license from [My Ig account](https://www.instagram.com/elie_marteau/).

By offering the plugin under GPL, the community benefits from free access, collaboration and learning. Commercial licenses allow you to sell it and protect your own proprietary code while still benefiting from the same high‑quality DSP core. Both models can coexist – you do not lose the right to sell simply because the code is also open source.

> 💡 JUCE itself is licensed under the same dual‑licensing model. You must comply with JUCE’s license when building and distributing this plugin.

---

## Credits

- **DSP & Concept** – Ely 
- **3D Visualizer** – Custom octahedron with particle system
- **Built with** [JUCE](https://juce.com)

---

## Roadmap (Ideas)

- Add more grain shapes (triangular, exponential windows)
- LFO synced to host transport
- More granular parameters exposed (spread, density, grain size)
- MIDI control (play grains as a polysynth)
- Improve performance under heavy grain load

---

*Have fun cooking chaotic sounds!*
