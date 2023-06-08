#设置第三方依赖库 由于存在交叉编译需要指定不同的版本依赖库
mkdir -p build
cd build
deps_arm_dir=/Users/anker/Projects/3D_Projects/PrusaSlicer/deps/build_arm64/destdir
deps_x86_dir=/Users/anker/Projects/3D_Projects/PrusaSlicer/deps/build_x86/destdir
sign_dir=/Users/anker/Desktop/ankerStudio_build/sign
#编译x86
PROJECT_NAME=AnkerMake_alpha
BINARY_NAME=AnkerStudio
ALIAS_BINARY_NAME=ankermake_alpha
ANKER_NET_NAME=libAnkerNet.dylib
export THIRD_PART_ROOT=$deps_arm_dir
cmake .. -DCMAKE_OSX_DEPLOYMENT_TARGET="10.14" -DCMAKE_OSX_ARCHITECTURES="arm64"  -DDESTDIR="./out"
cmake --build . -j8 --config Release
#cmake --install . --config Release

mkdir ./pack
mkdir ./pack/$PROJECT_NAME
mkdir ./pack/$PROJECT_NAME/$PROJECT_NAME.app
mkdir ./pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents
mkdir ./pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents/_CodeSignature
mkdir ./pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents/Frameworks
mkdir ./pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents/MacOS

cp  ../resources/crt/* ./pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents/MacOS
cp -Rf ../resources pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents/Resources
cp pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents/Resources/icons/$BINARY_NAME.icns pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents/resources/$BINARY_NAME.icns
cp $sign_dir/Info.plist pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents/Info.plist
echo -n -e 'APPL????\x0a' > pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents/PkgInfo
find pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents/Resources/localization -name "*.po" -type f -delete
cp -f src/$BINARY_NAME pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents/MacOS/$BINARY_NAME
#cp -f src/$ALIAS_BINARY_NAME pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents/MacOS/$ALIAS_BINARY_NAME
cp -f src/$ANKER_NET_NAME pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents/Frameworks/$ANKER_NET_NAME
#@executable_path/../Frameworks
install_name_tool -change @rpath/libAnkerNet.dylib @executable_path/../Frameworks/libAnkerNet.dylib pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents/MacOS/$BINARY_NAME

