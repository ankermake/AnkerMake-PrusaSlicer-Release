# CLAUDE.md - eufyMake Studio Development Guide for AI Assistants

**Last Updated:** 2025-12-08
**Version:** 1.5.26
**License:** GNU Affero General Public License v3

---

## Table of Contents
1. [Project Overview](#project-overview)
2. [Repository Structure](#repository-structure)
3. [Build System](#build-system)
4. [Dependencies Management](#dependencies-management)
5. [Development Workflows](#development-workflows)
6. [Codebase Architecture](#codebase-architecture)
7. [Key Technologies](#key-technologies)
8. [Coding Conventions](#coding-conventions)
9. [Testing Infrastructure](#testing-infrastructure)
10. [Common Tasks](#common-tasks)
11. [Important Notes for AI Assistants](#important-notes-for-ai-assistants)

---

## Project Overview

### What is eufyMake Studio?

eufyMake Studio (formerly AnkerMake Studio) is a professional 3D printing slicer application offering:
- Advanced slicing features with G-Code generation and visualization
- Remote printing control and monitoring
- Real-time video streaming from connected printers
- Multi-platform support (Windows, macOS x86/ARM, Linux)

### Heritage & Lineage

```
Slic3r (Alessandro Ranellucci, RepRap Community)
  └── PrusaSlicer (Prusa Research)
      └── eufyMake Studio (AnkerMake/eufyMake)
```

The project is based on PrusaSlicer by Prusa Research, which itself originated from Slic3r. This means:
- **Core algorithms** are proven and battle-tested
- **Architecture patterns** follow established 3D printing software conventions
- **Code style** may vary between legacy Slic3r code and newer Anker-specific features

### Project Statistics
- **Current Version:** 1.5.26
- **Source Code Size:** ~45MB
- **GUI Code:** 153,241 lines of C++ (GUI components alone)
- **Supported Languages:** 22 (en, zh_CN, zh_TW, ja, ko, de, fr, es, it, ru, pl, tr, uk, cs, nl, pt_BR, hu, ca)
- **File Formats:** 3MF, STL, OBJ, AMF, STEP, SVG, G-Code

---

## Repository Structure

### Root Directory Layout

```
eufyMake-PrusaSlicer-Release/
├── .git/                    # Git repository
├── .gitignore              # Git ignore rules
├── LICENSE                 # GNU AGPL v3 license
├── README.md               # Project overview
└── AnkerStudio/            # Main application directory
    ├── CMakeLists.txt      # Root CMake build file (718 lines)
    ├── version.inc         # Version definitions
    ├── gen_xcode_proj.sh   # macOS XCode project generator
    ├── cmake/              # CMake modules and utilities
    ├── deps/               # Third-party dependency build system
    ├── doc/                # Build documentation
    ├── pack/               # Packaging resources
    ├── resources/          # Application resources (icons, profiles, shaders)
    ├── src/                # Source code (27 subdirectories)
    └── tools/              # Development tools
```

### Source Code Organization (`AnkerStudio/src/`)

The source code is organized into **27 major directories**:

#### Core Slicing Engine

```
src/libslic3r/               # Core slicing library (8.3MB)
├── Algorithm/               # Region expansion, optimization
├── Arachne/                 # Advanced wall generation
│   ├── BeadingStrategy/     # Wall width distribution
│   └── utils/               # Half-edge graphs, junctions
├── BranchingTree/           # Tree support structures
├── CSGMesh/                 # Constructive solid geometry
├── Execution/               # Parallel execution framework
├── Fill/                    # Infill pattern generators
│   ├── FillRectilinear.cpp  # Grid patterns
│   ├── FillGyroid.cpp       # Gyroid infill
│   ├── FillHoneycomb.cpp    # Honeycomb pattern
│   ├── FillAdaptive.cpp     # Adaptive cubic (74KB)
│   └── Lightning/           # Lightning infill
├── Format/                  # File I/O (568KB total)
│   ├── 3mf.cpp (187KB)      # 3MF format support
│   ├── bbs_3mf.cpp (209KB)  # Bambu 3MF variant
│   ├── AMF.cpp, STL.cpp     # Mesh formats
│   ├── STEP.cpp             # CAD format via OCCT
│   └── SL1*.cpp             # SLA formats
├── GCode/                   # G-Code generation (801KB)
│   ├── GCodeProcessor.cpp   # Parser/analyzer (222KB)
│   ├── AvoidCrossingPerimeters.cpp
│   ├── CoolingBuffer.cpp    # Print cooling
│   ├── PressureEqualizer.cpp
│   ├── SeamPlacer.cpp       # Seam positioning
│   ├── SpiralVase.cpp       # Vase mode
│   └── WipeTower.cpp        # Multi-material
├── Geometry/                # Geometric algorithms
├── Optimize/                # Optimization routines
├── Orientation/             # Auto-orientation
└── SLA/                     # Resin printing support
```

**Key libslic3r files:**
- `Print.cpp`, `PrintObject.cpp` - Main print orchestration
- `PrintConfig.cpp` - Configuration management
- `Model.cpp` - 3D model representation
- `TriangleMesh.cpp`, `TriangleMeshSlicer.cpp` - Mesh processing
- `PerimeterGenerator.cpp` - Wall generation
- `SupportMaterial.cpp`, `TreeSupport.cpp` - Support structures
- `Flow.cpp`, `Extruder.cpp` - Extrusion calculations
- `Arrange.cpp` - Auto-arrange on build plate

#### GUI Layer

```
src/slic3r/                  # GUI application (11MB)
├── GUI/                     # Main UI components
│   ├── Anker*/              # Custom Anker widgets
│   │   ├── AnkerBtn.cpp/.h
│   │   ├── AnkerCheckBox.cpp/.h
│   │   ├── AnkerCfgTab.cpp (266KB)
│   │   ├── AnkerDevice.cpp (99KB)
│   │   ├── AnkerGCodeImportDialog.cpp (66KB)
│   │   ├── AnkerObjectBar.cpp (150KB)
│   │   └── AnkerNet*/       # Network integration
│   ├── 3DScene.cpp (78KB)   # 3D visualization
│   ├── 3DBed.cpp, 2DBed.cpp # Build plate rendering
│   ├── Plater.cpp           # Main workspace
│   ├── MainFrame.cpp        # Application window
│   ├── GLCanvas3D.cpp       # OpenGL rendering
│   ├── Preferences.cpp      # Settings dialog
│   ├── Tab.cpp              # Configuration tabs
│   ├── ConfigWizard.cpp     # First-run wizard
│   └── Jobs/                # Background tasks
├── Config/                  # Configuration system
└── Utils/                   # Utility functions
```

#### Third-Party Libraries (Embedded)

```
src/admesh/                  # STL repair (129KB)
src/agg/                     # Anti-Grain Geometry (457KB)
src/avrdude/                 # AVR programmer (12MB)
src/boost/                   # Boost.nowide (UTF-8)
src/clipper/                 # 2D polygon clipping
src/eigen/                   # Linear algebra (4.8MB headers)
src/glu-libtess/             # OpenGL tessellation (245KB)
src/hidapi/                  # USB HID devices
src/imgui/                   # Immediate mode GUI (2.5MB)
src/libigl/                  # Geometry processing (5.2MB)
src/libnest2d/               # 2D nesting (252KB)
src/miniz/                   # ZIP compression (418KB)
src/fast_float/              # Fast float parsing
src/qoi/                     # Image format
src/semver/                  # Semantic versioning
src/tcbspan/                 # C++ span
```

#### Platform & Support

```
src/platform/                # Platform abstraction
├── msw/                     # Windows-specific
├── osx/                     # macOS-specific
└── unix/                    # Linux-specific
src/exception_handle/        # Crash handling
src/occt_wrapper/            # OpenCascade wrapper (STEP)
src/anker_plungin/           # Plugin interface
src/build-utils/             # Build helpers
```

---

## Build System

### CMake Configuration

**Minimum CMake Version:** 3.13
**C++ Standard:** C++17 (required)
**Build Generator:** Visual Studio 2019+ (Windows), XCode (macOS), Make/Ninja (Linux)

### Key Build Options

```cmake
# Core options
SLIC3R_STATIC=ON              # Static linking (default ON for Win/Mac)
SLIC3R_GUI=ON                 # Build GUI (default ON)
SLIC3R_FHS=OFF                # FHS directory structure (default OFF)
SLIC3R_PCH=ON                 # Precompiled headers (default ON)

# Development options
SLIC3R_BUILD_TESTS=OFF        # Enable unit tests
SLIC3R_BUILD_SANDBOXES=OFF    # Development sandboxes
SLIC3R_PERL_XS=OFF            # Legacy Perl module

# Feature flags
SLIC3R_ENABLE_FORMAT_STEP=ON  # STEP file support (default ON)
ANKER_CLOUD_SLICE=OFF         # Cloud slicing (default OFF)
OPEN_SOURCE=OFF               # Open source mode (default OFF)

# Debug options
SLIC3R_ASAN=OFF               # AddressSanitizer
SLIC3R_UBSAN=OFF              # UndefinedBehaviorSanitizer
SLIC3R_PROFILE=OFF            # Shiny profiler integration

# MSVC-specific
SLIC3R_MSVC_COMPILE_PARALLEL=ON  # Parallel compilation
SLIC3R_MSVC_PDB=ON               # Generate PDB in Release
```

### Build Process Overview

#### 1. Build Dependencies First

```bash
# Navigate to deps directory
cd AnkerStudio/deps
mkdir deps_build
cd deps_build

# Windows
cmake -DCMAKE_BUILD_TYPE=Release -S .. -G "Visual Studio 16 2019"
# Build in Visual Studio or: msbuild ALL_BUILD.vcxproj

# macOS/Linux
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

Dependencies install to: `AnkerStudio/deps/deps_build/destdir/`

#### 2. Set Environment Variable

**CRITICAL:** Set `THIRD_PART_ROOT` to the **absolute path** of dependency install directory:

```bash
# Windows
set THIRD_PART_ROOT=C:\src\AnkerStudio\deps\deps_build\destdir

# macOS/Linux
export THIRD_PART_ROOT=/path/to/AnkerStudio/deps/deps_build/destdir
```

#### 3. Generate IDE Project

```bash
cd AnkerStudio
mkdir build
cd build

# Windows
cmake -DCMAKE_BUILD_TYPE=Release -S .. -G "Visual Studio 16 2019"

# macOS (use helper script)
../gen_xcode_proj.sh

# Linux
cmake -DCMAKE_BUILD_TYPE=Release ..
```

#### 4. Build Application

**Windows:** Open `AnkerStudio.sln` in Visual Studio, set `AnkerStudio_app_console` as startup project, build.

**macOS:** Open `AnkerStudio.xcodeproj` in XCode, select `AnkerStudio` scheme, build.

**Linux:** `make -j$(nproc)`

### Generated Executables

**Windows:**
- `eufyStudio.dll` - Core shared library
- `eufymake studio.exe` - GUI application
- `eufymake studio-console.exe` - Console version
- `anker-gcodeviewer.exe` - Standalone G-code viewer

**macOS/Linux:**
- `eufyStudio` - Main executable
- Symlinks: `eufymake studio`, `anker-gcodeviewer`, `AnkerGCodeViewer`

### Compilation Notes

- **Parallel Compilation:** `/MP` on MSVC, `-jN` on Unix
- **Build Time:** Initial build can take 30-60 minutes depending on hardware
- **Disk Space:** ~10GB for full build with dependencies
- **Incremental Builds:** Typically 1-5 minutes for code changes
- **Precompiled Headers:** Enabled by default, significantly speeds up rebuilds

---

## Dependencies Management

### External Dependencies (26 Libraries)

The `AnkerStudio/deps/` directory uses CMake ExternalProject to build all dependencies from source.

#### Core Libraries

| Library | Version | Purpose |
|---------|---------|---------|
| **Boost** | 1.66.0+ | system, filesystem, thread, log, locale, regex, iostreams |
| **TBB** | Latest | Intel Threading Building Blocks (parallel execution) |
| **Eigen3** | 3.3+ | Linear algebra, matrix operations |
| **CGAL** | Latest | Computational geometry algorithms |

#### Graphics & UI

| Library | Version | Purpose |
|---------|---------|---------|
| **wxWidgets** | 3.2 | Cross-platform GUI (html, adv, gl, core, webview) |
| **OpenGL** | 2.0+ | 3D rendering (legacy profile) |
| **GLEW** | Latest | OpenGL extension wrangling |
| **OpenCSG** | Latest | Constructive Solid Geometry rendering |

#### File I/O & Compression

| Library | Purpose |
|---------|---------|
| **ZLIB** | General compression |
| **PNG** | PNG image format |
| **JPEG** | JPEG image format |
| **TIFF** | TIFF image format |
| **EXPAT** | XML parsing |
| **NanoSVG** | SVG parsing |
| **Blosc** | Fast compression for OpenVDB |
| **OpenEXR** | HDR image format |

#### Geometry & Math

| Library | Version | Purpose |
|---------|---------|---------|
| **Qhull** | 7.2+ | Convex hull, Delaunay triangulation |
| **NLopt** | 1.4+ | Nonlinear optimization |
| **OpenVDB** | 5.0+ | Sparse volumetric data structures |
| **OCCT** | Latest | OpenCascade (STEP file support) |
| **GMP** | Latest | Arbitrary precision arithmetic |
| **MPFR** | Latest | Multiple-precision floating-point |

#### Networking

| Library | Purpose |
|---------|---------|
| **CURL** | HTTP/HTTPS client |
| **OpenSSL** | TLS/SSL, cryptography |

#### Serialization

| Library | Purpose |
|---------|---------|
| **Cereal** | C++ serialization library |

### Runtime Dependencies (Not Built by deps/)

These must be installed separately:

| Library | Purpose | Install Location |
|---------|---------|------------------|
| **FFmpeg** | Video streaming (avcodec, avformat, avutil, swresample, swscale) | System/build dir |
| **jansson** | JSON parsing | System/build dir |
| **Paho MQTT** | MQTT messaging (paho-mqtt3cs.dll) | deps/bin/win/ |
| **PPCS_API.dll** | P2P connection service | deps/bin/win/ |
| **WebView2Loader** | Embedded Chromium browser (Windows) | pack/windows/runtime/ |

### Dependency Platform Files

- `deps/deps-windows.cmake` - Windows-specific dependencies
- `deps/deps-macos.cmake` - macOS-specific dependencies
- `deps/deps-linux.cmake` - Linux-specific dependencies
- `deps/deps-mingw.cmake` - MinGW cross-compilation
- `deps/deps-unix-common.cmake` - Common Unix dependencies

---

## Development Workflows

### Git Workflow

#### Commit Message Conventions

Based on recent commit history:
```
feat: <description>           # New features
fix: <description>            # Bug fixes
docs: <description>           # Documentation changes
refactor: <description>       # Code refactoring
```

Examples from history:
- `feat: fix conflict error`
- `feat: 1.5.21 open source update newest`
- `feat: open source v1.5.24`

#### Branching Strategy

- **Main Branches:** `release_AnkerStudio`, `dev_AnkerStudio`
- **Feature Branches:** `dev_<feature>_<version>`, `dev_open_source_<version>`
- **Merge Requests:** Used for integration (e.g., "Accept Merge Request #22217")

#### Version Management

Version is defined in `AnkerStudio/version.inc`:
```cmake
set(SLIC3R_APP_NAME "eufyMake Studio")
set(SLIC3R_VERSION "1.5.26")
set(SLIC3R_MIN_VER "")  # Internal iteration like "_1"
set(SLIC3R_BUILD_ID "eufyMake Studio-${SLIC3R_VERSION}${SLIC3R_MIN_VER}")
set(SLIC3R_RC_VERSION "1,5,26,0")
```

### Development Environment Setup

#### Windows

**Prerequisites:**
- Visual Studio 2019+ with C++ Desktop Development workload
- CMake 3.13+
- Git for Windows
- FFmpeg (download from ffmpeg.org)
- OpenSSL (download from openssl.org)
- jansson (build from source)

**Setup Steps:**
1. Clone repository close to drive root (avoid spaces, non-ASCII paths)
2. Build dependencies: `cd deps/deps_build && cmake .. && build`
3. Set `THIRD_PART_ROOT` environment variable (absolute path)
4. Install FFmpeg, OpenSSL, jansson to system or build directory
5. Generate VS project: `cd build && cmake .. -G "Visual Studio 16 2019"`
6. Copy runtime files to `build/src/Debug` or `build/src/Release`:
   - `AnkerStudio/resources/crt/*.crt`
   - `AnkerStudio/deps/bin/win/x64/*.dll`
   - `AnkerStudio/pack/windows/runtime/WebView2Loader.dll`
   - FFmpeg DLLs, OpenSSL DLLs
7. Open `AnkerStudio.sln`, set startup project to `AnkerStudio_app_console`
8. Build → Rebuild Solution (required once per config)
9. Debug → Start Debugging (F5)

**Network Plugin (Release mode only):**
Install via GUI menu to enable device control and streaming.

#### macOS

**Prerequisites:**
- XCode (from App Store)
- CMake, Git, gettext (via Homebrew: `brew install cmake git gettext`)
- FFmpeg, OpenSSL, jansson

**Setup Steps:**
1. Clone repository (avoid spaces in path)
2. Build dependencies: `cd deps/deps_build && cmake .. && make -j$(sysctl -n hw.ncpu)`
3. Edit `gen_xcode_proj.sh`: Update `deps_arm_dir` to absolute path of `deps/deps_build/destdir`
4. Install FFmpeg, OpenSSL, jansson
5. Generate XCode project: `./gen_xcode_proj.sh`
6. Create output directory: `mkdir -p build_xcode/src/Release`
7. Open `build_xcode/AnkerStudio.xcodeproj` in XCode
8. Select `AnkerStudio` scheme, set Build Configuration to Release
9. Build and run

**Network Plugin:**
- **x86:** Copy `deps/bin/mac/x86/*` to `build_xcode/src/Frameworks/`, then install via GUI
- **ARM (M1/M2):** Copy `deps/bin/mac/arm/*` to `~/Library/Application Support/AnkerMake Studio Profile/OnlineAnkerNet/Current/`

#### Linux

Not officially documented, but follows Unix patterns:
1. Install dependencies via package manager (build-essential, cmake, git, wx-widgets, opengl, etc.)
2. Build deps as in macOS
3. Generate Makefiles: `cmake -DCMAKE_BUILD_TYPE=Release ..`
4. Build: `make -j$(nproc)`

### Debugging

**Debug Builds:**
- Enable with `-DCMAKE_BUILD_TYPE=Debug`
- Full symbols included
- Assertions enabled
- No optimization

**Release Builds with Debug Info (Windows):**
```cmake
CMAKE_CXX_FLAGS_RELEASE: "/Zi"
CMAKE_EXE_LINKER_FLAGS_RELEASE: "/DEBUG:FULL /OPT:REF /OPT:ICF"
```
Generates PDB files even in Release for post-mortem debugging.

**Sanitizers (Clang/GCC):**
- AddressSanitizer: `-DSLIC3R_ASAN=ON`
- UndefinedBehaviorSanitizer: `-DSLIC3R_UBSAN=ON`

### Localization Workflow

**Translation Files:** `AnkerStudio/resources/localization/`

**Process:**
1. Extract strings: Use gettext tools to extract from source → `AnkerMake Studio.pot` (429KB)
2. Translate: Edit .po files for each language (22 languages supported)
3. Compile: `msgfmt` compiles .po → .mo binary catalogs
4. Import: Use `tools/importPoFiles.js` for batch operations

**Supported Languages:**
ca, cs, de, en, es, fr, hu, it, ja, ko, ko_KR, nl, pl, pt_BR, ru, tr, uk, zh_CN, zh_TW

---

## Codebase Architecture

### Architectural Layers

```
┌─────────────────────────────────────────────────────┐
│           AnkerStudio (GUI Application)             │
│  ┌───────────────────┐  ┌────────────────────────┐ │
│  │  Custom Anker UI  │  │   Standard GUI         │ │
│  │  - AnkerBtn       │  │   - Plater             │ │
│  │  - AnkerDevice    │  │   - MainFrame          │ │
│  │  - AnkerCfgTab    │  │   - 3DScene            │ │
│  │  - AnkerNetModule │  │   - GLCanvas3D         │ │
│  └───────────────────┘  └────────────────────────┘ │
├─────────────────────────────────────────────────────┤
│              wxWidgets + OpenGL + ImGui             │
├─────────────────────────────────────────────────────┤
│           libslic3r (Core Slicing Engine)           │
│  ┌─────────────┐ ┌──────────┐ ┌─────────────────┐ │
│  │ Print       │ │ GCode    │ │ Geometry        │ │
│  │ Model       │ │ Fill     │ │ Arachne         │ │
│  │ Config      │ │ Support  │ │ Format I/O      │ │
│  └─────────────┘ └──────────┘ └─────────────────┘ │
├─────────────────────────────────────────────────────┤
│        Geometry Libraries (Eigen, libigl, CGAL)     │
├─────────────────────────────────────────────────────┤
│         Platform Layer (Windows/macOS/Linux)        │
└─────────────────────────────────────────────────────┘
```

### Key Design Patterns

#### 1. Model-View Architecture

**Model Layer (libslic3r):**
- `Model.cpp` - Represents 3D objects, instances, volumes
- `Print.cpp` - Orchestrates slicing pipeline
- `PrintObject.cpp` - Per-object slicing logic
- `PrintConfig.cpp` - Configuration hierarchy

**View Layer (slic3r/GUI):**
- `Plater.cpp` - Main workspace (arranges models, shows preview)
- `GLCanvas3D.cpp` - OpenGL rendering of 3D scene
- `Tab.cpp` - Configuration UI tabs
- `MainFrame.cpp` - Top-level window

**Controller:**
- GUI components observe model changes
- User actions trigger model updates
- Slicing runs asynchronously via Jobs system

#### 2. Configuration System

Three-level hierarchy:
1. **PrintConfig** - Print settings (layer height, infill, supports)
2. **PrinterConfig** - Printer capabilities (build volume, nozzle size)
3. **MaterialConfig** - Filament properties (temperature, cooling)

Implemented as:
- `ConfigBase` - Base class with option definitions
- `DynamicConfig` - Runtime configuration
- `PrintConfig`, `PrintObjectConfig`, etc. - Typed configs

#### 3. Slicing Pipeline

```cpp
// High-level flow (simplified)
Model → Print → PrintObject → PrintRegion → LayerRegion → ExtrusionPaths → GCode
```

Steps:
1. **Model Import** - Load STL/3MF/etc. → `TriangleMesh`
2. **Slicing** - `TriangleMeshSlicer` → `ExPolygons` per layer
3. **Perimeter Generation** - `PerimeterGenerator` → walls
4. **Infill Generation** - `Fill::fill_surface()` → infill patterns
5. **Support Generation** - `SupportMaterial` or `TreeSupport`
6. **G-Code Generation** - `GCode::do_export()` → output file

#### 4. Jobs System

Background task execution (`slic3r/GUI/Jobs/`):
- **Job** - Base class for long-running tasks
- **PlaterJob** - Jobs that affect the plater
- **ProgressIndicator** - UI progress feedback
- **Thread Pool** - Powered by TBB

Example jobs: Slicing, auto-arrange, orientation, thumbnail generation

#### 5. Plugin Architecture

**AnkerNetModule:**
- Dynamic loading of network plugin DLL/dylib
- Interface defined in `anker_plungin/`
- Handles device communication, MQTT, P2P streaming
- Proprietary in closed-source builds, stubbed in `OPEN_SOURCE` mode

### Critical Code Paths

#### Slicing a Model

**Entry Point:** `Plater::priv::background_process()` → `BackgroundSlicingProcess`

**Sequence:**
1. Validate configuration
2. Auto-arrange if needed
3. Generate support structures
4. Slice each object layer-by-layer
5. Generate toolpaths (perimeters, infill)
6. Calculate print time estimate
7. Export G-code

**Key Files:**
- `slic3r/GUI/Plater.cpp` - User-facing orchestration
- `libslic3r/Print.cpp` - Process() method drives slicing
- `libslic3r/GCode.cpp` - Final G-code output

#### 3D Rendering

**Entry Point:** `GLCanvas3D::render()` in `slic3r/GUI/GLCanvas3D.cpp`

**Components:**
- **3DScene** - Scene graph management
- **GLVolume** - Renderable volumes (models, supports, toolpaths)
- **GLShaderProgram** - Shader management (resources/shaders/)
- **ImGui Overlays** - On-screen controls and info

**Shader Versions:**
- GLSL 1.10 (resources/shaders/110/) - Legacy OpenGL 2.0
- GLSL 1.40 (resources/shaders/140/) - OpenGL 3.1
- GLSL ES (resources/shaders/ES/) - Embedded systems

#### File I/O

**Import:** `Model::load()` → `Format::load_*()` functions
**Export:** `Print::export_gcode()` → `GCode::do_export()`

**Supported Formats:**
- **3MF (Primary):** `Format/3mf.cpp` (187KB) - Full material/config support
- **STL:** `Format/STL.cpp` - Binary and ASCII
- **OBJ:** `Format/OBJ.cpp` - Wavefront
- **AMF:** `Format/AMF.cpp` - Additive Manufacturing Format
- **STEP:** `Format/STEP.cpp` via OCCT wrapper
- **G-Code:** `GCode/GCodeProcessor.cpp` (222KB) - Parser and viewer

---

## Key Technologies

### Programming Languages

- **C++17** - Primary language (all core code)
- **C** - Some legacy libraries (admesh, miniz)
- **JavaScript** - Tools (importPoFiles.js)
- **Shell** - Build scripts (gen_xcode_proj.sh)
- **CMake** - Build system configuration

### Frameworks & Libraries

#### GUI & Graphics

| Technology | Version | Usage |
|------------|---------|-------|
| **wxWidgets** | 3.2 | Cross-platform GUI framework (all dialogs, windows, controls) |
| **OpenGL** | 2.0+ | 3D rendering (legacy profile for compatibility) |
| **GLEW** | Latest | OpenGL extension loading |
| **ImGui** | 2.5MB | Immediate mode GUI (overlays on 3D view) |
| **AGG** | 457KB | 2D vector graphics (Anti-Grain Geometry) |

#### Math & Geometry

| Library | Usage |
|---------|-------|
| **Eigen3** | Matrix operations, linear algebra, transforms |
| **libigl** | Mesh boolean operations, geometry processing |
| **CGAL** | Computational geometry, exact arithmetic |
| **Qhull** | Convex hull computation, Delaunay triangulation |
| **Clipper** | 2D polygon clipping, offsetting (for perimeters) |
| **OpenVDB** | Volumetric data (adaptive infill, mesh voxelization) |

#### File I/O & Compression

| Library | Usage |
|---------|-------|
| **miniz** | ZIP compression (3MF files are ZIP archives) |
| **Blosc** | High-performance compression for OpenVDB data |
| **ZLIB** | General compression |
| **libpng, libjpeg, libtiff** | Image formats (thumbnails, textures) |
| **NanoSVG** | SVG parsing (build plate textures) |
| **EXPAT** | XML parsing (AMF, 3MF metadata) |

#### Networking & Communication

| Library | Usage |
|---------|-------|
| **libcurl** | HTTP/HTTPS requests (firmware updates, profiles) |
| **OpenSSL** | TLS/SSL encryption, certificate validation |
| **Paho MQTT** | MQTT messaging protocol (device communication) |
| **PPCS API** | Proprietary P2P connection service |
| **FFmpeg** | Video encoding/decoding (camera streaming) |
| **WebView2** | Embedded Chromium browser (Windows only) |

#### Parallelization

| Library | Usage |
|---------|-------|
| **Intel TBB** | Task-based parallelism, parallel_for, concurrent containers |
| **Boost.Thread** | Threading utilities, thread pools |
| **C++17 Parallel Algorithms** | std::execution::par for STL algorithms |

#### Other

| Library | Usage |
|---------|-------|
| **Cereal** | C++ object serialization |
| **jansson** | JSON parsing (network API responses) |
| **Boost** | nowide (UTF-8 file paths), filesystem, system, regex |
| **fast_float** | Fast string-to-float conversion |
| **semver** | Semantic versioning comparison |

### Development Tools

- **CMake 3.13+** - Build system generator
- **Git** - Version control
- **Visual Studio 2019+** - Windows IDE
- **XCode** - macOS IDE
- **GCC/Clang** - Linux compilers
- **gettext** (xgettext, msgfmt, msgmerge) - Internationalization

---

## Coding Conventions

### Style Guidelines

**Note:** This codebase inherits from Slic3r/PrusaSlicer, so style may vary between legacy and new code. There is no `.clang-format` or `.editorconfig` in the repository.

#### Observed Conventions (from codebase inspection)

**Naming:**
- **Classes:** PascalCase (e.g., `PrintObject`, `GCodeProcessor`, `AnkerDevice`)
- **Methods:** snake_case (e.g., `process_layer()`, `generate_perimeters()`)
- **Variables:** snake_case (e.g., `layer_height`, `extrusion_width`)
- **Constants:** UPPER_SNAKE_CASE or k-prefix (e.g., `MAX_LAYER_HEIGHT`, `kDefaultSpeed`)
- **Member variables:** Often `m_` prefix (e.g., `m_config`, `m_layers`)

**File Organization:**
- **Header files:** `.h` or `.hpp`
- **Source files:** `.cpp`
- **Typical structure:** One class per file pair (ClassName.h + ClassName.cpp)
- **Large subsystems:** Organized in subdirectories (e.g., `GCode/`, `Format/`, `Arachne/`)

**Indentation:**
- Appears to be **4 spaces** (no tabs in most files)

**Braces:**
- Opening brace on same line for functions: `void foo() {`
- Opening brace on same line for control flow: `if (condition) {`

**Comments:**
- C++ style preferred: `// Comment`
- Doxygen not consistently used
- Important algorithms have block comments

**Includes:**
- System headers first: `#include <vector>`
- Project headers next: `#include "Print.hpp"`
- Relative paths for local includes: `#include "../libslic3r.h"`

**Namespaces:**
- Primary namespace: `Slic3r::`
- Nested namespaces: `Slic3r::GUI::`, `Slic3r::Geometry::`
- Using declarations avoided in headers

### Code Quality Practices

**Error Handling:**
- Exceptions used for error propagation (`std::runtime_error`, `Slic3r::RuntimeError`)
- Critical errors thrown, not silently ignored
- File I/O errors always checked

**Memory Management:**
- Modern C++ preferred: `std::unique_ptr`, `std::shared_ptr`
- Legacy code may use raw pointers
- RAII pattern used for resource cleanup

**Const Correctness:**
- Const references for large objects: `const Model& model`
- Const methods marked: `size_t size() const`
- Const wherever possible

**Threading:**
- TBB used for parallelism
- Mutexes protect shared data (`std::mutex`, `tbb::mutex`)
- Jobs system for background tasks

### Platform-Specific Code

**Conditional Compilation:**
```cpp
#ifdef _WIN32
    // Windows code
#elif __APPLE__
    // macOS code
#else
    // Linux code
#endif
```

**Platform Abstraction:**
- Use `platform/` directory for OS-specific implementations
- Keep platform-specific code isolated
- Prefer cross-platform APIs when available

### Configuration

**CMake Options:**
- Check for existing options before adding new ones
- Document new options in CMakeLists.txt comments
- Follow naming convention: `SLIC3R_*` prefix

**Version Updates:**
- Modify `version.inc` for version changes
- Follow semantic versioning: MAJOR.MINOR.PATCH
- Update `SLIC3R_RC_VERSION` for Windows resources

### GUI Development

**Anker Custom Widgets:**
- Prefix with `Anker*` (e.g., `AnkerBtn`, `AnkerDialog`)
- Inherit from wxWidgets base classes
- Follow existing Anker widget patterns

**Standard GUI:**
- Use wxWidgets idioms
- Event binding: `Bind(wxEVT_*, &Class::OnEvent, this)`
- Layout: Use sizers (wxBoxSizer, wxFlexGridSizer)

**3D Rendering:**
- OpenGL 2.0 legacy profile for compatibility
- Shader code in `resources/shaders/`
- Support multiple GLSL versions (110, 140, ES)

### Localization

**String Externalization:**
```cpp
// Use L() macro for user-facing strings
wxString msg = L("Print complete!");

// Use _L() for already-translated literals
wxString label = _L("Temperature");
```

**Translation Files:**
- Strings extracted to `AnkerMake Studio.pot`
- Translations in `resources/localization/<lang>/*.po`
- Use gettext tools for extraction

---

## Testing Infrastructure

### Current Status

**Testing is present but minimally used:**
- Framework: **Catch2** (modern C++ testing)
- CMake integration: `CatchAddTests.cmake`, `ParseAndAddCatchTests.cmake`
- Build flag: `SLIC3R_BUILD_TESTS=OFF` (default)

**Existing Tests:**
- `libigl/igl/pseudonormal_test.cpp/h` - Geometry tests
- Calibration models in `resources/calib/` - Physical test prints

**Legacy:**
- `SLIC3R_PERL_XS=OFF` - Old Perl test suite (disabled)

### Running Tests (if enabled)

```bash
cmake -DSLIC3R_BUILD_TESTS=ON ..
make
ctest --verbose
```

### Test Development Guidance

**If you need to add tests:**

1. **Location:** Create tests in `tests/` subdirectory or alongside source
2. **Framework:** Use Catch2
3. **Naming:** `*_test.cpp`
4. **Structure:**
```cpp
#include <catch2/catch.hpp>
#include "YourClass.hpp"

TEST_CASE("YourClass does something", "[yourclass]") {
    YourClass obj;
    REQUIRE(obj.method() == expected);
}
```
5. **CMake:** Add test executable and register with CTest

### Calibration & Physical Testing

**Calibration Prints:** `resources/calib/`
- **Filament Flow:** `filament_flow/*.3mf` - Test extrusion multiplier
- **Pressure Advance:** `pressure_advance/*.stl` - Tune pressure advance
- **Volumetric Speed:** `volumetric_speed/*.stl` - Find max flow rate

**Usage:** Open in GUI, slice, print, measure results

---

## Common Tasks

### Task: Add a New Infill Pattern

**Location:** `AnkerStudio/src/libslic3r/Fill/`

**Steps:**
1. Create new file: `FillYourPattern.cpp` and `FillYourPattern.hpp`
2. Inherit from `Fill` base class
3. Implement `_fill_surface_single()` method
4. Register in `Fill.cpp` factory
5. Add to `InfillPattern` enum in `PrintConfig.hpp`
6. Add UI string in localization
7. Update CMakeLists.txt

**Example:**
```cpp
// FillYourPattern.hpp
#pragma once
#include "Fill.hpp"

namespace Slic3r {

class FillYourPattern : public Fill {
public:
    Fill* clone() const override { return new FillYourPattern(*this); }
    void _fill_surface_single(
        const FillParams& params,
        unsigned int thickness_layers,
        const std::pair<float, Point>& direction,
        ExPolygon expolygon,
        Polylines& polylines_out) override;
};

} // namespace Slic3r
```

### Task: Add a New File Format

**Location:** `AnkerStudio/src/libslic3r/Format/`

**Steps:**
1. Create `YourFormat.cpp` and `YourFormat.hpp`
2. Implement `load_yourformat()` and/or `store_yourformat()` functions
3. Update `Model::load()` to recognize file extension
4. Update `Print::export_yourformat()` if export format
5. Add to supported formats list in GUI
6. Update file dialogs with new extension

**Key Functions:**
```cpp
bool load_yourformat(const char* path, Model* model);
bool store_yourformat(const char* path, const Model& model);
```

### Task: Add a Configuration Option

**Location:** `AnkerStudio/src/libslic3r/PrintConfig.cpp`

**Steps:**
1. Add to appropriate config class (`PrintConfig`, `PrintObjectConfig`, etc.)
2. Define option in config constructor:
```cpp
def = this->add("your_option", coFloat);
def->label = L("Your Option");
def->tooltip = L("Description of what this does");
def->min = 0;
def->max = 100;
def->default_value = new ConfigOptionFloat(50.0);
```
3. Add getter/setter if needed
4. Add UI control in `slic3r/GUI/Tab.cpp`
5. Use in slicing logic (e.g., `Print.cpp`, `GCode.cpp`)
6. Add to profile files: `resources/profiles/Anker-ini/`

### Task: Add a Custom GUI Widget

**Location:** `AnkerStudio/src/slic3r/GUI/`

**Steps:**
1. Create `AnkerYourWidget.cpp` and `AnkerYourWidget.hpp`
2. Inherit from wxWidgets base (e.g., `wxPanel`, `wxControl`)
3. Implement constructor, event handlers
4. Override `OnPaint()` for custom rendering
5. Add to parent window/dialog
6. Update CMakeLists.txt

**Example:**
```cpp
// AnkerYourWidget.hpp
#pragma once
#include <wx/panel.h>

class AnkerYourWidget : public wxPanel {
public:
    AnkerYourWidget(wxWindow* parent);

private:
    void OnPaint(wxPaintEvent& event);
    void OnClick(wxMouseEvent& event);

    wxDECLARE_EVENT_TABLE();
};
```

### Task: Update Localization

**Steps:**
1. Wrap user-facing strings with `L()` macro:
```cpp
wxString msg = L("New feature message");
```
2. Extract strings to POT:
```bash
cd AnkerStudio/resources/localization
xgettext --keyword=L --language=C++ -o "AnkerMake Studio.pot" --files-from=list.txt
```
3. Update PO files:
```bash
msgmerge --update <lang>/AnkerMake_Studio.po "AnkerMake Studio.pot"
```
4. Edit PO files (translate new strings)
5. Compile MO files:
```bash
msgfmt <lang>/AnkerMake_Studio.po -o <lang>/AnkerMake_Studio.mo
```
6. Test in GUI (change language in Preferences)

### Task: Add a Printer Profile

**Location:** `AnkerStudio/resources/profiles/Anker-ini/`

**Steps:**
1. Create directory: `Anker-ini/YourPrinter/`
2. Create `printer.ini` with printer capabilities:
```ini
[printer]
name = Your Printer
bed_shape = 0x0,200x0,200x200,0x200
max_print_height = 200
nozzle_diameter = 0.4
```
3. Create subdirectories for nozzle sizes: `0.4mm/`, `0.6mm/`, etc.
4. In each, create `print.ini` with print profiles:
```ini
[print]
layer_height = 0.2
perimeters = 3
infill_density = 20%
```
5. Add 3D model: `Anker/YourPrinter-bed.stl`
6. Add thumbnail: `Anker/YourPrinter.png`
7. Add build plate texture: `Anker/YourPrinter_texture.svg`
8. Test loading in Configuration Wizard

### Task: Modify G-Code Generation

**Location:** `AnkerStudio/src/libslic3r/GCode.cpp`

**Key Method:** `GCode::_do_export()`

**Steps:**
1. Identify insertion point in G-code sequence
2. Modify `GCode::_do_export()` or helper methods
3. Use `gcodegen.writer()` for output:
```cpp
gcode += gcodegen.writer().travel_to_xy(Point(x, y));
gcode += gcodegen.writer().extrude_to_xy(Point(x, y), e);
```
4. Consider speed, acceleration, temperature changes
5. Test with sample model
6. Verify G-code in viewer

### Task: Debug a Slicing Issue

**Approach:**
1. Identify affected model (STL/3MF)
2. Enable Debug build: `cmake -DCMAKE_BUILD_TYPE=Debug ..`
3. Set breakpoint in relevant code:
   - Slicing: `TriangleMeshSlicer::slice()`
   - Perimeters: `PerimeterGenerator::process()`
   - Infill: `Fill::fill_surface()`
   - Supports: `SupportMaterial::generate()`
4. Load model, start slicing
5. Step through code, inspect variables
6. Check intermediate results (ExPolygons, Polylines)
7. Export debug geometry if needed

**Logging:**
```cpp
#include <iostream>
std::cout << "Debug: layer " << layer_id << ", region " << region_id << std::endl;
```

### Task: Profile Performance

**Option 1: Built-in Profiler**
```bash
cmake -DSLIC3R_PROFILE=ON ..
```
Uses Shiny profiler (invasive instrumentation).

**Option 2: External Tools**
- **Windows:** Visual Studio Performance Profiler, Intel VTune
- **macOS:** Instruments (Time Profiler)
- **Linux:** perf, Valgrind (callgrind)

**Hotspots:** Slicing, toolpath generation, G-code export, 3D rendering

---

## Important Notes for AI Assistants

### Critical Reminders

1. **THIRD_PART_ROOT Must Be Absolute**
   - Never use relative paths like `../deps/deps_build/destdir`
   - Always use full absolute paths: `C:\src\AnkerStudio\deps\deps_build\destdir`

2. **Build Dependencies First**
   - Always build `deps/` before main application
   - Dependencies install to `deps/deps_build/destdir/`
   - Main build will fail without dependencies

3. **Runtime Files Required**
   - Copy DLLs to build output directory (Windows)
   - Copy certificates from `resources/crt/`
   - Copy network plugin from `deps/bin/`
   - Copy WebView2Loader.dll (Windows)

4. **Network Plugin is Release-Only**
   - Device control requires Release build
   - Network plugin won't load in Debug mode

5. **Cross-Compilation Disables Tests**
   - ARM builds on x86 Mac disable tests automatically
   - Don't try to run tests on cross-compiled builds

6. **Version String is Generated**
   - Edit `version.inc`, not source files directly
   - CMake propagates version to all components

7. **Localization Requires gettext**
   - xgettext, msgfmt, msgmerge must be in PATH
   - PO files must be compiled to MO for use

8. **OpenGL is Legacy Profile**
   - Code targets OpenGL 2.0 for compatibility
   - Don't use modern OpenGL 4.x features
   - Shaders are GLSL 1.10/1.40, not 3.30+

### When Modifying Code

**DO:**
- Read existing code patterns before adding new code
- Follow established naming conventions (PascalCase classes, snake_case methods)
- Use existing base classes (Fill, GLVolume, Job, etc.)
- Check CMakeLists.txt when adding new files
- Test on actual hardware/printers when possible
- Profile performance for compute-intensive code
- Validate G-code output with a G-code viewer
- Consider backward compatibility with existing profiles

**DON'T:**
- Mix tabs and spaces (use spaces)
- Add proprietary dependencies without approval
- Break API compatibility without versioning
- Introduce platform-specific code in cross-platform files
- Ignore compiler warnings (especially C++17 deprecations)
- Commit large binary files (use Git LFS if needed)
- Hardcode paths or assumptions about directory structure

### Understanding the Codebase

**Start Here:**
1. **For GUI changes:** `slic3r/GUI/Plater.cpp` (main workspace)
2. **For slicing logic:** `libslic3r/Print.cpp` (orchestration)
3. **For G-code:** `libslic3r/GCode.cpp` (output generation)
4. **For file I/O:** `libslic3r/Format/` (import/export)
5. **For geometry:** `libslic3r/Geometry/` (algorithms)

**Key Concepts:**
- **Model** - 3D objects before slicing
- **Print** - Slicing orchestrator
- **PrintObject** - Individual sliced object
- **Layer** - Horizontal slice of object
- **Region** - Area with same print settings
- **ExPolygon** - Polygon with holes (perimeters, infill boundaries)
- **Polyline** - Ordered points (toolpaths)
- **ExtrusionPath** - Polyline with extrusion data (width, flow, speed)
- **GCode** - Final output format

**Data Flow:**
```
STL/3MF → TriangleMesh → Model → Print → Slice → ExPolygons →
ExtrusionPaths → GCode → File
```

### Common Pitfalls

1. **Forgetting to Build Dependencies**
   - Symptom: Linker errors, missing headers
   - Solution: Build `deps/` first, set `THIRD_PART_ROOT`

2. **Path Length Issues (Windows)**
   - Symptom: Build fails with strange errors
   - Solution: Clone closer to drive root (e.g., `C:\src\`)

3. **Missing Runtime DLLs (Windows)**
   - Symptom: Application crashes on startup
   - Solution: Copy all DLLs to output directory

4. **wxWidgets ABI Mismatch**
   - Symptom: Crashes in GUI code
   - Solution: Rebuild deps with same compiler/settings as main app

5. **OpenGL Context Issues**
   - Symptom: Black screen, crash in GLCanvas3D
   - Solution: Ensure wxGLCanvas initialized before GL calls

6. **Configuration Not Updating**
   - Symptom: Changes to .ini files not reflected
   - Solution: Delete `AppData/eufyMake Studio/` cache, reload

7. **Localization Not Working**
   - Symptom: English strings shown for other languages
   - Solution: Compile PO to MO, place in `resources/localization/`

### Security Considerations

**This is AGPL-licensed software:**
- Any modifications must be released under AGPL v3
- If used over network, source must be available to users
- Proprietary plugins (AnkerNet) are separate licensed components

**Handling User Data:**
- Printer profiles may contain proprietary information
- G-code may reveal build techniques
- Network communication requires SSL/TLS
- Credentials stored securely (use system keychain/credential manager)

**File Parsing:**
- STL/3MF may be malformed or malicious
- Validate mesh data (triangle count, vertex bounds)
- Protect against ZIP bombs in 3MF files
- Sanitize filenames from archives

### Debugging Tips

**Enable Verbose Logging:**
- Check for log output in `AppData/eufyMake Studio/` (Windows) or `~/Library/Application Support/eufyMake Studio/` (Mac)

**Visualize Geometry:**
- Export intermediate ExPolygons to SVG for inspection
- Use `export_surface_type_legend_to_svg_file_named()`

**Simplify Test Case:**
- Start with simple cube, isolate issue
- Reduce layer count, disable supports/infill
- Binary search settings to identify culprit

**Compare with PrusaSlicer:**
- Same model, same settings, compare output
- Identify divergence point

**Use Sanitizers:**
- AddressSanitizer catches memory errors
- UndefinedBehaviorSanitizer catches UB
- Run test models through sanitized builds

### Performance Optimization

**Hotspots (from profiling):**
1. **TriangleMeshSlicer::slice()** - Mesh slicing (CPU-bound)
2. **PerimeterGenerator::process()** - Perimeter generation
3. **Fill::fill_surface()** - Infill pattern generation
4. **GCode::_do_export()** - G-code string generation
5. **GLCanvas3D::render()** - 3D rendering (GPU-bound)

**Optimization Strategies:**
- Use TBB parallel_for for layer-wise operations
- Precompute geometry when possible
- Avoid unnecessary memory allocations
- Use move semantics for large objects
- Profile before optimizing (don't guess)

### Resources

**Official Documentation:**
- Build Guides: `AnkerStudio/doc/Windows_build.md`, `MacOs_build.md`
- Repository: https://github.com/eufymake/eufyMake-PrusaSlicer-Release

**Upstream Projects:**
- PrusaSlicer: https://github.com/prusa3d/PrusaSlicer
- Slic3r: https://github.com/Slic3r/Slic3r

**Libraries:**
- wxWidgets: https://docs.wxwidgets.org/
- Eigen: https://eigen.tuxfamily.org/
- ImGui: https://github.com/ocornut/imgui
- CGAL: https://doc.cgal.org/
- libigl: https://libigl.github.io/

**G-Code Reference:**
- Marlin: https://marlinfw.org/meta/gcode/
- RepRap: https://reprap.org/wiki/G-code

---

## Summary

eufyMake Studio is a sophisticated, mature 3D printing slicer application with:

- **Well-architected codebase** - Clear separation of slicing engine (libslic3r) and GUI (slic3r)
- **Comprehensive dependencies** - 26+ external libraries managed via CMake
- **Multi-platform support** - Windows, macOS (x86/ARM), Linux
- **Advanced slicing** - Arachne walls, lightning infill, tree supports
- **Rich GUI** - Custom Anker widgets, 3D visualization, real-time preview
- **Network features** - Remote printing, device management, video streaming
- **Extensive localization** - 22 languages
- **Professional tooling** - CMake, gettext, shader support
- **Open Source Heritage** - Built on PrusaSlicer/Slic3r foundation

**For AI Assistants:** This document provides the context needed to understand the codebase structure, navigate the code, and make informed modifications. Always read existing code patterns before making changes, follow established conventions, and test thoroughly on target platforms.

---

**End of CLAUDE.md**
