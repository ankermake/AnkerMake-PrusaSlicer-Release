## AnkerStudio

## Overview
AnkerMake Studio is based on PrusaSlicer 2.6.0.alpha6

## What are AnkerStudio's main features?

# Key features are:
	1. Basic slicing features & GCode viewer;
	2. Multiple plates management;
	3. Remote control & monitoring;
	4. multi-platform (Win/Mac/Linux) support;
	
## How to compile?

# Compile and debug projects using visual studio on windows platform:
	1. cd deps;mkdir deps_build;cd deps_build;
	2. cmake  -DCMAKE_BUILD_TYPE=Debug -S .. -G "Visual Studio 16 2019";
	3. Open the project and build the dependency library: {path}/AnkerStudio/deps/deps_build/destdir;
	4. Install some other dependency library: jansson;
	5. Set system environment variables:  THIRD_PART_ROOT = {path}/AnkerStudio/deps/deps_build/destdir;
	6. mkdir AnkerStudio/build && AnkerStudio/build;
	7. cmake -DCMAKE_BUILD_TYPE=Debug -S .. -G "Visual Studio 16 2019";
	8. Open the project and build;
	
# Building projects on Mac platform:
	1. cd deps;mkdir deps_build;cd deps_build;
	2. cmake ..;
	3. make -j16;
	4. build dependency library: {path}/AnkerStudio/deps/deps_build/destdir;
	5. open build_arm.sh and update deps_arm_dir={path}/AnkerStudio/deps/deps_build/destdir;
	6. ./build_arm.sh build code;

## License
	Anker Studio is licensed under the _GNU Affero General Public License, version 3_. 
	Anker Studio is based on PrusaSlicer by PrusaResearch.
	The PrusaSlicer is originally based on Slic3r by Alessandro Ranellucci.