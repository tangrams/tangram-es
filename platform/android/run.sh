adb uninstall com.mapzen.tangram.android
adb install android/demo/build/outputs/apk/demo-debug.apk
adb shell am start -a android.intent.action.MAIN -n com.mapzen.tangram.android/.MainActivity
