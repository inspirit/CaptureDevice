
MY_DIR=${0%/*}
source $MY_DIR/config.cfg

# ANE packaging
ANE_NAME=capture.ane

#===================================================================

echo "****** creating ANE package *******"

rm ./ane/${ANE_NAME}

rm -rf build.tmp
mkdir build.tmp
mkdir build.tmp/as
mkdir build.tmp/windows
mkdir build.tmp/osx
mkdir build.tmp/ios
mkdir build.tmp/android
mkdir build.tmp/android/libs
mkdir build.tmp/android/libs/armeabi

cp ./lib/CaptureInterface.swc ./build.tmp/as/CaptureInterface.zip
unzip -d ./build.tmp/as/ ./build.tmp/as/CaptureInterface.zip

#windows
cp build.tmp/as/library.swf build.tmp/windows
cp ./native/win32/capane.dll ./build.tmp/windows

#osx
cp build.tmp/as/library.swf build.tmp/osx
cp -r ./native/osx/captureOSX.framework ./build.tmp/osx

#ios
cp build.tmp/as/library.swf build.tmp/ios
cp -r ./native/ios/libcaptureIOS.a ./build.tmp/ios

#android
cp build.tmp/as/library.swf build.tmp/android
#cp -r ./native/android/libs/armeabi-v7a/libcapture.so ./build.tmp/android
cp -r ./native/java/capture.jar ./build.tmp/android
cp -r ./native/java/libs/armeabi-v7a/libcolor_convert.so ./build.tmp/android/libs/armeabi
##
#cp -r ./native/android/jni/armeabi-v7a/libnative_camera_r2.2.0.so ./build.tmp/android/libs/armeabi
#cp -r ./native/android/jni/armeabi-v7a/libnative_camera_r2.3.3.so ./build.tmp/android/libs/armeabi
#cp -r ./native/android/jni/armeabi-v7a/libnative_camera_r3.0.1.so ./build.tmp/android/libs/armeabi
#cp -r ./native/android/jni/armeabi-v7a/libnative_camera_r4.0.0.so ./build.tmp/android/libs/armeabi
#cp -r ./native/android/jni/armeabi-v7a/libnative_camera_r4.0.3.so ./build.tmp/android/libs/armeabi

adt -package \
		-target ane \
		./ane/${ANE_NAME} \
		./extension.xml \
		-swc ./lib/CaptureInterface.swc \
		-platform Windows-x86 -C ./build.tmp/windows/ . \
		-platform MacOS-x86 -C ./build.tmp/osx/ . \
        -platform iPhone-ARM -platformoptions ./native/ios/platform.xml -C ./build.tmp/ios/ . \
		-platform Android-ARM -C ./build.tmp/android/ . 

echo ""

#rm ./dist/${ANE_NAME}
#cp ./ane/${ANE_NAME} ./dist/
rm -rf build.tmp

echo "DONE"
