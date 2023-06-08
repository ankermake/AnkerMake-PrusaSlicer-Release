

pkg_version=$1
#设置第三方依赖库 由于存在交叉编译需要指定不同的版本依赖库
deps_x86_dir=/Users/anker/Projects/3D_Projects/PrusaSlicer/deps/build_x86/destdir

#export THIRD_PART_ROOT=$PWD/dependents/destdir
#
scripyDir=$PWD
sign_dir=$PWD/sign
tempDate=$(date +"%Y%m%d%s")
tempPath=build_x86_dir_$tempDate

mkdir $tempPath
cd ./$tempPath

#拉去代码并指定branch
branch="feature_ankermake_alpha"
while getopts ":b:" opt; do
  case ${opt} in
    b )
      branch=${OPTARG}
      ;;
    \? )
      echo "Invalid option: -$OPTARG" 1>&2
      exit 1
      ;;
    : )
      echo "Option -$OPTARG requires an argument." 1>&2
      exit 1
      ;;
  esac
done

echo "Pulling from branch $branch"
git clone -b $branch http://e.coding.anker-in.com/codingcorp/zz_3d_platform/AnkerSlicer_P.git

cd ./AnkerSlicer_P/AnkerStudio
build_dir=$PWD
x86_build_path=$PWD/x86_build
mkdir $x86_build_path

#编译x86
PROJECT_NAME=AnkerMake_alpha
BINARY_NAME=AnkerStudio
ALIAS_BINARY_NAME=ankermake_alpha
ANKER_NET_NAME=libAnkerNet.dylib
pushd $x86_build_path
export THIRD_PART_ROOT=$deps_x86_dir
cmake .. -DCMAKE_OSX_DEPLOYMENT_TARGET="10.14" -DCMAKE_OSX_ARCHITECTURES="x86_64"  -DDESTDIR="./out"
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
cp ${THIRD_PART_ROOT}/usr/local/lib/FFmpeg/*.dylib           ./pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents/Frameworks/

install_name_tool -change @rpath/libhwy.1.dylib    @executable_path/../Frameworks/libhwy.1.dylib    pack/$PROJECT_NAME/$PROJECT_NAME.app/Contents/Frameworks/libhwy_contrib.1.dylib
popd

echo  curent_dir :$PWD

#copy x64 pack
cp -rf $x86_build_path/pack/$PROJECT_NAME .

#签名
echo "start sign "
pushd $sign_dir
app_location="$build_dir/$PROJECT_NAME/$PROJECT_NAME.app"
dev_acc=ankerdev2019@gmail.com
dev_pwd=tsqg-udus-cfwe-rqeq
dev_team="Developer ID Application: Power Mobile Life LLC (BVL93LPC7F)"
dev_entitlements="entitlements.plist"
bundle_id="com.anker.pcankermake"
dmgnametemp="$PROJECT_NAME.tmp.dmg"
pkg_exec="$build_dir/$PROJECT_NAME.app/Contents/MacOS/$BINARY_NAME"
codesign -dvv --verbose=4 -f -s "$dev_team" -v "$app_location" --deep --entitlements "$dev_entitlements" -o runtime
popd

#封装成dmg
cp $sign_dir/* .
mkdir build
mv $PROJECT_NAME/$PROJECT_NAME.app build/
/opt/homebrew/bin/node   "/opt/homebrew/lib/node_modules/appdmg/bin/appdmg.js" appdmg.json $dmgnametemp

res_file="/tmp/result.tmp"
uuid_file="/tmp/uuid.tmp"

echo "Upload AnkerMake.dmg............."
#上传苹果服务器做公证
xcrun altool --notarize-app -f "$dmgnametemp" --primary-bundle-id $bundle_id -u $dev_acc -p $dev_pwd > $uuid_file
cat -n $uuid_file

#if [ $upload_result == 0 ];then
UUID=$(grep "RequestUUID" $uuid_file|awk '{print $3}')

if [ "$UUID" != "" ]; then
echo "---> Checking $UUID"
echo "Upload Success!!!"
else
echo "---> UUID ERROR!!!"
exit -1
fi

isSuccess=0
statusError=0;

while true; do
rm -rf $res_file
isSuccess=0
statusError=0;
#获取公证结果
xcrun altool --notarization-info $UUID -u $dev_acc -p $dev_pwd > $res_file
cat -n $res_file
isSuccess=$(grep -c "Status Message: Package Approved" $res_file)
statusError=$(grep -c "Status: invalid" $res_file)
statusError+=$(grep -c "Status Message: Package Invalid" $res_file)
if [ $isSuccess -gt 0 ]; then
echo "---> Package Approved!!!"
#成功后处理dmg
xcrun stapler staple $dmgnametemp
echo "---> Make AnkerMake.dmg Success!!!"
destpkg=${PROJECT_NAME}_V${pkg_version}.dmg
mv $dmgnametemp $scripyDir/$tempPath/$destpkg
echo "---> uploading AnkerMake.dmg begin!!!"
curl -T $scripyDir/$tempPath/$destpkg -u ankermake_slicer-1682563095644:a84f68f1ab1b99a359131acaa03dcb97d46b8169 "http://codingcorp-generic.pkg.coding.anker-in.com/zz_3d_platform/ankermake_slicer/alpha_${pkg_version}/ankermake_slicer_mac/${destpkg}?version=${pkg_version}"
echo "---> uploading AnkerMake.dmg end!!!"
break
elif [ $statusError -gt 0 ]; then
echo "---> Package Invalid!!!"
echo "---> Make AnkerMake.dmg Failed!!!"
exit -1
break
else
echo "---> Package Processing!!!"
sleep 5s
fi
done
