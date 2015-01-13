adb uninstall com.mapzen.tangram
adb install android/bin/TangramAndroid-debug.apk
adb shell am start -a android.intent.action.MAIN -n com.mapzen.tangram/.MainActivity