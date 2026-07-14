# VocalOne

**VocalOne** è un plugin audio professionale per la lavorazione della voce, sviluppato in **C++17** con il framework **JUCE**. Fornisce una catena vocale completa "dall'ingresso al master" con analisi assistita da AI, correzione di intonazione, EQ intelligente, compressione program-dependent, effetti creativi e un assistente conversazionale integrato.

Formati supportati: **VST3**, **AU** (Audio Unit) e **Standalone**.
Target: **macOS (Apple Silicon)** e **Windows**.

---

## Caratteristiche principali

La catena di processamento è eseguita rigorosamente in quest'ordine:

| # | Modulo | Descrizione |
|---|--------|-------------|
| 1 | **AI Analyzer** | Analizza tonalità, BPM, genere, sesso della voce, range vocale, volume (RMS/Peak), rumore di fondo, respirazioni, dinamica, sibilanti e distanza dal microfono. |
| 2 | **AI Gate** | Noise gate automatico con soglia che segue il noise floor rilevato. |
| 3 | **Noise Removal** | Denoiser a sottrazione spettrale in stile iZotope RX, con apprendimento automatico del profilo di rumore. |
| 4 | **AI De-Esser** | De-esser split-band con soglia automatica basata sull'energia delle sibilanti. |
| 5 | **Smart Tune** | Correzione di intonazione in tempo reale con snapping a scala/tonalità, velocità e intensità regolabili. |
| 6 | **AI EQ** | Equalizzatore a 8 bande che rileva automaticamente Mud, Harsh, Nasal, Boxy, Thin, Boomy, Air e Presence. |
| 7 | **AI Compressor** | Compressore program-dependent che riconosce gli stili Rap, Pop, Rock, Podcast e Voice Over. |
| 8 | **Saturation** | 5 modelli analogici: Tube, Tape, Vintage, Warm, Digital (con oversampling). |
| 9 | **Harmonic Exciter** | Generatore di armoniche in alta frequenza (stile Aphex). |
| 10 | **Fresh Air** | Enhancer d'aria a doppio high-shelf (mid-air ~8 kHz, high-air ~16 kHz). |
| 11 | **Delay** | Delay stereo con feedback, ping-pong e filtro di damping. |
| 12 | **Reverb** | Riverbero con pre-delay e high-pass sulla coda. |
| 13 | **Doubler** | Doppiaggio artificiale con voci detunate e distribuite nello stereo. |
| 14 | **Chorus** | Chorus classico a delay modulato. |
| 15 | **Stereo / Width** | Controllo di larghezza mid/side con protezione mono dei bassi. |
| 16 | **Final Limiter** | Limiter brickwall con look-ahead e misurazione LUFS. |
| 17 | **AI Assistant** | Chat integrata con LLM (endpoint compatibile OpenAI) che riceve il contesto d'analisi. |
| 18 | **Smart Preset** | Preset di fabbrica + generatore di preset automatici basati sull'analisi. |

---

## Struttura del progetto

```
VocalOne/
├── CMakeLists.txt          # Configurazione build (VST3 + AU + Standalone)
├── README.md
├── Source/
│   ├── PluginProcessor.*   # AudioProcessor + parameter tree + catena
│   ├── PluginEditor.*      # Shell UI a schede
│   ├── DSP/                # 16 moduli DSP + analizzatore
│   ├── AI/                 # AIAssistant, SmartPreset
│   └── UI/                 # Pannelli, look&feel neon, meter e display
└── assets/
    └── fonts/              # (placeholder per font personalizzati)
```

---

## Requisiti

- **CMake** ≥ 3.22
- Un compilatore C++17 (Xcode/clang su macOS, MSVC su Windows, GCC/Clang su Linux)
- **JUCE** (clonato come cartella affiancata `../JUCE`)

---

## Compilazione

Dalla cartella che contiene sia `VocalOne/` che `JUCE/`:

```bash
# 1. Clona JUCE (se non presente)
git clone --depth=1 https://github.com/juce-framework/JUCE.git

# 2. Configura
cd VocalOne
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Compila
cmake --build build --config Release
```

Per indicare un percorso JUCE diverso:

```bash
cmake -B build -DJUCE_DIR=/percorso/di/JUCE
```

I binari (VST3/AU/Standalone) vengono generati nella cartella `build/VocalOne_artefacts/`.

---

## Assistente AI

L'assistente usa un endpoint compatibile con l'API OpenAI. Configura le variabili d'ambiente nell'host:

- `ABACUS_API_KEY` **oppure** `OPENAI_API_KEY` — la chiave API
- `OPENAI_BASE_URL` — (opzionale) URL base dell'endpoint

Senza chiave, tutti i moduli DSP e l'analizzatore continuano a funzionare: solo la chat resta disabilitata.

---

## Note tecniche

- Tutta l'elaborazione è a virgola mobile a 32 bit, compatibile con qualsiasi sample rate.
- I parametri sono gestiti tramite `AudioProcessorValueTreeState`, con serializzazione XML dello stato.
- L'analisi gira sul thread audio con pubblicazione lock-free dei risultati verso la GUI.
- Il codice è commentato in inglese secondo le convenzioni JUCE.
