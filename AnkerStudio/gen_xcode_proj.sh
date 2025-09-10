mkdir -p build_xcode
cd build_xcode
deps_arm_dir="/Users/anker/code/third_party_lib/arm_64"
export THIRD_PART_ROOT=$deps_arm_dir
cmake .. -GXcode -DCMAKE_PREFIX_PATH=$deps_arm_dir"/usr/local" -DSLIC3R_STATIC=1
crt_gd_ca_path="$(pwd)/../resources/crt/GD_CA.crt"
make_us_qa_path="$(pwd)/../resources/crt/make-us-qa.crt"
make_us_path="$(pwd)/../resources/crt/make-us.crt"
exec_path="$(pwd)/../build_xcode/src/Debug"
cp "$crt_gd_ca_path" "$exec_path"
cp "$make_us_qa_path" "$exec_path"
cp "$make_us_path" "$exec_path"