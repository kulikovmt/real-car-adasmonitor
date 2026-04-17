# Real Car ADAS Monitor

Multi-threaded automotive Advanced Driver Assistance System (ADAS) that simultaneously displays real-time OBD-II telemetry data and monitors driver state through a webcam — all rendered in a single 1280×480 window.

## What it Does

| Left Panel (Dashboard) | Right Panel (DMS Camera) |
|---|---|
| Speed gauge (0–140 km/h) | Face detection with corner brackets |
| Tachometer (0–6000 RPM) | Eye state: Open / Closed |
| Coolant temperature bar | Head direction: Front / Turned |
| Fuel level bar | Drowsiness alert banner |
| Throttle position bar | Distraction warning bar |
| Driving style (SLOW / NORMAL / AGGRESSIVE) | Real-time status indicators |

## Technology Stack

| Component | Technology | Version |
|---|---|---|
| Language | C++17 | MSVC 2022 |
| Build System | CMake | 3.20+ |
| Computer Vision | OpenCV | 4.9.0 |
| Neural Network Inference | ONNX Runtime | 1.17.1 |
| Face Detection | OpenCV DNN (Caffe SSD) | ResNet-10 |
| Eye Detection | OpenCV Haar Cascade | haarcascade_eye |
| Unit Testing | Google Test | latest (FetchContent) |
| Documentation | Doxygen | 1.9+ |

## Build Instructions

```bash
# Configure (from project root)
cmake -S . -B build

# Build
cmake --build build --config Debug

# Run tests
.\build\Debug\RunTests.exe
```

## How to Run

```bash
# Launch the full system (requires webcam)
.\build\Debug\RealCarMonitor.exe
```

### Controls
| Key | Action |
|---|---|
| `Q` / `ESC` | Exit |
| `SPACE` | Pause / Resume |
| `S` | Save screenshot to `output/screenshot.jpg` |

### Output Files
- `output/result_situation2.mp4` — Recorded video of the session
- `output/dms_alerts.log` — Timestamped alert log
- Console statistics printed on exit

## Architecture

```
┌────────────────────────────────────────────────────┐
│              Main Thread (30 FPS)                   │
│  VideoCapture → DMSMonitor → DMSHUD → Dashboard    │
│       ↕ SharedState (mutex-protected)               │
│              OBD Thread (10 Hz)                     │
│  CSV Reader → ONNXClassifier → SharedState          │
└────────────────────────────────────────────────────┘
```

## Results

| Metric | Value |
|---|---|
| Dashboard FPS | ~25-30 |
| DNN face detection | Every 5th frame (cached) |
| Eye detection | Every frame (Haar, <1ms) |
| Drowsiness trigger | Eyes closed in 20/30 frames |
| Distraction trigger | Head turn > 20° |
| OBD update rate | 10 Hz |
| Tests passed | 8/8 |

## Screenshots

Run the application and press `S` to capture screenshots demonstrating:
1. Normal operation (face detected, green brackets)
2. Drowsiness alert (eyes closed, orange banner)
3. Aggressive driving alert (from OBD telemetry)
