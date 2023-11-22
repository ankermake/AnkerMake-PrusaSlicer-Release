mkdir -p build_xcode
cd build_xcode
deps_arm_dir="/Users/anker/code/third_party_lib/arm_64"
export THIRD_PART_ROOT=$deps_arm_dir
cmake .. -GXcode -DCMAKE_PREFIX_PATH=$deps_arm_dir"/usr/local" -DSLIC3R_STATIC=1