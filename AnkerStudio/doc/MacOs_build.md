
# Building AnkerMake Studio on Mac OS

## 0. Prerequisities
To build AnkerMake Studio on Mac OS, you will need the following software:

- XCode from app store
- CMake
- Git
- gettext
- FFmepg
- openssl
- jansson

XCode is available through Apple's App Store, the other three tools are available on
[brew](https://brew.sh/) (use `brew install cmake git gettext` to install them).

## 1. Download sources
Clone the respository. Use a directory relatively close to the drive root, so the path is not too long. Avoid spaces and non-ASCII characters, run:
```
mkdir src
cd src
<<<<<<< HEAD
git clone git@github.com:ankermake/AnkerMake-PrusaSlicer-Release.git
=======
git clone https://github.com/ankermake/AnkerMake-PrusaSlicer-Release.git
>>>>>>> 84b4984 (feat: 1.5.21 open source)
```

## 2. Build Instructions

### Compile and Install the dependencies.
run the following:

<<<<<<< HEAD
	1. cd src/AnkerMake-PrusaSlicer-Release/AnkerStudio/deps
	2. mkdir src/AnkerMake-PrusaSlicer-Release/AnkerStudio/deps/deps_build
	3. cd src/AnkerMake-PrusaSlicer-Release/AnkerStudio/deps/deps_build
	4. cmake ..
	5. make -jN (N can be a number between 1 and the max cpu number)
	6. build the dependency library to src/AnkerMake-PrusaSlicer-Release/AnkerStudio/deps/deps_build/destdir
	7. Install some other dependency librarys: jansson, FFmepg, openssl

Note that `N` is the number of CPU cores, so, for example `make -j4` for a 4-core machine.
The library and include path for dependencies should reference `src/AnkerMake-PrusaSlicer-Release/AnkerStudio/src/slic3r/CMakeLists.txt`

### Generate XCode project file for AnkerMake Studio.

	1. update deps_arm_dir to src/AnkerMake-PrusaSlicer-Release/AnkerStudio/deps/deps_build/destdir in src/AnkerMake-PrusaSlicer-Release/AnkerStudio/gen_xcode_proj.sh 
	2. mkdir -p src/AnkerMake-PrusaSlicer-Release/AnkerStudio/build_xcode/src/Debug
	3. bash src/AnkerMake-PrusaSlicer-Release/gen_xcode_proj.sh to generate XCode project

Note that `deps_arm_dir` must be absolute path. A relative path will not work.
Note that `mkdir -p src/AnkerMake-PrusaSlicer-Release/AnkerStudio/build_xcode/src/Debug` change to `mkdir -p src/AnkerMake-PrusaSlicer-Release/AnkerStudio/build_xcode/src/Release` when you select release build.

### Compile and Run AnkerMake Studio. 
Click `src/AnkerMake-PrusaSlicer-Release/AnkerStudio/build_xcode/AnkerStudio.xcodeproj` to open it in XCode (This should open up XCode where you can perform build using the GUI or perform other tasks.).
=======
	1. cd src/AnkerStudio/deps
	2. mkdir src/AnkerStudio/deps/deps_build
	3. cd src/AnkerStudio/deps/deps_build
	4. cmake ..
	5. make -jN
	6. build the dependency library to src/AnkerStudio/deps/deps_build/destdir
	7. Install some other dependency librarys: jansson, FFmepg, openssl

Note that `N` is the number of CPU cores, so, for example `make -j4` for a 4-core machine.
The library and include path for dependencies should reference `AnkerStudio\src\slic3r\CMakeLists.txt`

### Generate XCode project file for AnkerMake Studio.

	1. update deps_arm_dir to src/AnkerStudio/deps/deps_build/destdir in src/AnkerStudio/gen_xcode_proj.sh 
	2. mkdir -p src/AnkerStudio/build_xcode/src/Debug
	3. run the src/gen_xcode_proj.sh to generate XCode project

Note that `deps_arm_dir` must be absolute path. A relative path will not work.
Note that `mkdir -p src/AnkerStudio/build_xcode/src/Debug` change to `mkdir -p src/AnkerStudio/build_xcode/src/Release` when you select release build.

### Compile and Run AnkerMake Studio. 
Click `src\AnkerStudio\build_xcode\AnkerStudio.xcodeproj` to open it in XCode (This should open up XCode where you can perform build using the GUI or perform other tasks.).
>>>>>>> 84b4984 (feat: 1.5.21 open source)

Select `AnkerStudio` as your startup project, as shown in the figure below:

![Alt text](Image/xcode_set_ankerstudio2.jpg)  

![Alt text](Image/xcode_select_ankerstudio.jpg)

Click the run button to compile and run AnkerMake Studio.

![Alt text](Image/xcode_click_run.jpg)

AnkerMake Studio should start. You're up and running!

### Network Plugin
<<<<<<< HEAD

### x86 Architecture Computer
=======
>>>>>>> 84b4984 (feat: 1.5.21 open source)
If you need to use network functions or device control functions, then you need to install the network plug-in, as shown in the figure below:

![Alt text](Image/install_network_plugin.png)

<<<<<<< HEAD
Before downloading the network plug-in library, you need to perform the following operations:

	1. mkdir -p src/AnkerMake-PrusaSlicer-Release/AnkerStudio/build_xcode/src/Frameworks
	2. cp src/AnkerMake-PrusaSlicer-Release/AnkerStudio/deps/bin/mac/x86/* src/AnkerMake-PrusaSlicer-Release/AnkerStudio/build_xcode/src/Frameworks

### arm Architecture Computer
On ARM-based computers, such as M1/M2, etc, no need to click the install button to download the network plug-in library. 
You need to perform the following operations:

	1. mkdir -p ~/Library/Application Support/AnkerMake Studio Profile/OnlineAnkerNet/Current
	2. cp src/AnkerMake-PrusaSlicer-Release/AnkerStudio/deps/bin/mac/arm/* ~/Library/Application Support/AnkerMake Studio Profile/OnlineAnkerNet/Current

Then reopen the software and you can use it.

### Use Release Mode
=======
>>>>>>> 84b4984 (feat: 1.5.21 open source)
Note that AnkerMake Studio must use Release mode when downloading and using network plug-ins.

How to use release mode refer to the figure below：

![Alt text](Image/xcode_set_scheme.jpg)

![Alt text](Image/xcode_set_release.jpg)