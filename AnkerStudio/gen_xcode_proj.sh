# Resolve paths relative to this script so the tree can live in any directory
# (e.g. a fresh fork clone) without editing hardcoded paths.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mkdir -p "$SCRIPT_DIR/build_xcode"
cd "$SCRIPT_DIR/build_xcode"
deps_arm_dir="$SCRIPT_DIR/deps/deps_build/destdir"
export THIRD_PART_ROOT=$deps_arm_dir
cmake .. -GXcode -DCMAKE_PREFIX_PATH=$deps_arm_dir"/usr/local" -DSLIC3R_STATIC=1 -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_OSX_ARCHITECTURES=arm64
crt_gd_ca_path="$(pwd)/../resources/crt/GD_CA.crt"
make_us_qa_path="$(pwd)/../resources/crt/make-us-qa.crt"
make_us_path="$(pwd)/../resources/crt/make-us.crt"
exec_path="$(pwd)/../build_xcode/src/Debug"
# Ensure the output dir exists so the cp's below create files *inside* it rather
# than a file literally named "Debug" (which then breaks linking of libAnkerNet).
mkdir -p "$exec_path"
cp "$crt_gd_ca_path" "$exec_path"
cp "$make_us_qa_path" "$exec_path"
cp "$make_us_path" "$exec_path"