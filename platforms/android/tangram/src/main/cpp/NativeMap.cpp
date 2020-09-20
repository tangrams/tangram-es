#include "AndroidMap.h"
#include "AndroidPlatform.h"
#include "JniHelpers.h"
#include "JniThreadBinding.h"
#include <android/bitmap.h>
#include <tangram.h>

namespace Tangram {

AndroidMap* androidMapFromJava(JNIEnv* env, jobject nativeMapObject) {
    static jclass nativeMapClass = env->FindClass("com/mapzen/tangram/NativeMap");
    static jfieldID nativePointerFID = env->GetFieldID(nativeMapClass, "nativePointer", "J");
    jlong nativePointer = env->GetLongField(nativeMapObject, nativePointerFID);
    assert(nativePointer > 0);
    return reinterpret_cast<AndroidMap*>(nativePointer);
}

std::vector<Tangram::SceneUpdate> unpackSceneUpdates(JNIEnv* jniEnv, jobjectArray updateStrings) {
    int nUpdateStrings = (updateStrings == nullptr)? 0 : jniEnv->GetArrayLength(updateStrings);

    std::vector<Tangram::SceneUpdate> sceneUpdates;
    for (int i = 0; i < nUpdateStrings;) {
        auto path = (jstring) (jniEnv->GetObjectArrayElement(updateStrings, i++));
        auto value = (jstring) (jniEnv->GetObjectArrayElement(updateStrings, i++));
        sceneUpdates.emplace_back(JniHelpers::stringFromJavaString(jniEnv, path), JniHelpers::stringFromJavaString(jniEnv, value));
        jniEnv->DeleteLocalRef(path);
        jniEnv->DeleteLocalRef(value);
    }
    return sceneUpdates;
}

extern "C" {

#define NATIVE_METHOD(NAME) JNIEXPORT JNICALL Java_com_mapzen_tangram_NativeMap_ ## NAME

jlong NATIVE_METHOD(init)(JNIEnv* env, jobject obj, jobject mapController, jobject assetManager) {
    auto map = new AndroidMap(env, mapController, assetManager);
    return reinterpret_cast<jlong>(map);
}

void NATIVE_METHOD(dispose)(JNIEnv* env, jobject obj) {
    auto* map = androidMapFromJava(env, obj);
    delete map;
}

void NATIVE_METHOD(shutdown)(JNIEnv* env, jobject obj) {
    auto* map = androidMapFromJava(env, obj);
    map->getPlatform().shutdown();
}

void NATIVE_METHOD(onLowMemory)(JNIEnv* env, jobject obj) {
    auto* map = androidMapFromJava(env, obj);
    map->onMemoryWarning();
}

jint NATIVE_METHOD(loadScene)(JNIEnv* env, jobject obj, jstring path,
                              jobjectArray updateStrings) {
    auto* map = androidMapFromJava(env, obj);

    auto cPath = JniHelpers::stringFromJavaString(env, path);

    auto sceneUpdates = unpackSceneUpdates(env, updateStrings);
    Url sceneUrl = Url("asset:///").resolve(Url(cPath));
    jint sceneId = map->loadScene(sceneUrl.string(), false, sceneUpdates);

    return sceneId;
}

jint NATIVE_METHOD(loadSceneAsync)(JNIEnv* env, jobject obj, jstring path,
                                   jobjectArray updateStrings) {
    auto* map = androidMapFromJava(env, obj);

    auto cPath = JniHelpers::stringFromJavaString(env, path);

    auto sceneUpdates = unpackSceneUpdates(env, updateStrings);
    Url sceneUrl = Url("asset:///").resolve(Url(cPath));
    jint sceneId = map->loadSceneAsync(sceneUrl.string(), false, sceneUpdates);

    return sceneId;
}

jint NATIVE_METHOD(loadSceneYaml)(JNIEnv* env, jobject obj, jstring yaml, jstring path,
                                  jobjectArray updateStrings) {
    auto* map = androidMapFromJava(env, obj);

    auto cYaml = JniHelpers::stringFromJavaString(env, yaml);
    auto cPath = JniHelpers::stringFromJavaString(env, path);

    auto sceneUpdates = unpackSceneUpdates(env, updateStrings);
    Url sceneUrl = Url("asset:///").resolve(Url(cPath));
    jint sceneId = map->loadSceneYaml(cYaml, sceneUrl.string(), false, sceneUpdates);

    return sceneId;
}

jint NATIVE_METHOD(loadSceneYamlAsync)(JNIEnv* env, jobject obj, jstring yaml, jstring path,
                                       jobjectArray updateStrings) {
    auto* map = androidMapFromJava(env, obj);

    auto cYaml = JniHelpers::stringFromJavaString(env, yaml);
    auto cPath = JniHelpers::stringFromJavaString(env, path);

    auto sceneUpdates = unpackSceneUpdates(env, updateStrings);
    Url sceneUrl = Url("asset:///").resolve(Url(cPath));
    jint sceneId = map->loadSceneYamlAsync(cYaml, sceneUrl.string(), false, sceneUpdates);

    return sceneId;
}

void NATIVE_METHOD(setupGL)(JNIEnv* env, jobject obj) {
    auto* map = androidMapFromJava(env, obj);
    map->setupGL();
}

void NATIVE_METHOD(resize)(JNIEnv* env, jobject obj, jint width, jint height) {
    auto* map = androidMapFromJava(env, obj);
    map->resize(width, height);
}

jint NATIVE_METHOD(update)(JNIEnv* env, jobject obj, jfloat dt) {
    auto* map = androidMapFromJava(env, obj);
    auto result = map->update(dt);
    return static_cast<jint>(result.flags);
}

void NATIVE_METHOD(render)(JNIEnv* env, jobject obj) {
    auto* map = androidMapFromJava(env, obj);
    map->render();
}

void NATIVE_METHOD(captureSnapshot)(JNIEnv* env, jobject obj, jintArray buffer) {
    auto* map = androidMapFromJava(env, obj);
    jint* ptr = env->GetIntArrayElements(buffer, nullptr);
    auto* data = reinterpret_cast<unsigned int*>(ptr);
    map->captureSnapshot(data);
    env->ReleaseIntArrayElements(buffer, ptr, 0);
}

void NATIVE_METHOD(getCameraPosition)(JNIEnv* env, jobject obj, jobject cameraPositionOut) {
    auto* map = androidMapFromJava(env, obj);
    CameraPosition cameraPosition = map->getCameraPosition();
    JniHelpers::cameraPositionToJava(env, cameraPositionOut, cameraPosition);
}

void NATIVE_METHOD(updateCameraPosition)(JNIEnv* env, jobject obj,
                                         jint set, jdouble lon, jdouble lat,
                                         jfloat zoom, jfloat zoomBy,
                                         jfloat rotation, jfloat rotateBy,
                                         jfloat tilt, jfloat tiltBy,
                                         jdouble b1lon, jdouble b1lat,
                                         jdouble b2lon, jdouble b2lat,
                                         jobject javaPadding, jfloat duration, jint ease) {
    auto* map = androidMapFromJava(env, obj);

    CameraUpdate update;
    update.set = set;

    update.lngLat = LngLat{lon,lat};
    update.zoom = zoom;
    update.zoomBy = zoomBy;
    update.rotation = rotation;
    update.rotationBy = rotateBy;
    update.tilt = tilt;
    update.tiltBy = tiltBy;
    update.bounds = std::array<LngLat,2>{{LngLat{b1lon, b1lat}, LngLat{b2lon, b2lat}}};
    update.padding = JniHelpers::edgePaddingFromJava(env, javaPadding);
    map->updateCameraPosition(update, duration, static_cast<Tangram::EaseType>(ease));
}

void NATIVE_METHOD(getEnclosingCameraPosition)(JNIEnv* env, jobject obj,
                                               jobject javaLngLatSE, jobject javaLngLatNW,
                                               jobject javaPadding, jobject cameraPositionOut) {
    auto* map = androidMapFromJava(env, obj);

    EdgePadding padding = JniHelpers::edgePaddingFromJava(env, javaPadding);
    LngLat lngLatSE = JniHelpers::lngLatFromJava(env, javaLngLatSE);
    LngLat lngLatNW = JniHelpers::lngLatFromJava(env, javaLngLatNW);
    CameraPosition camera = map->getEnclosingCameraPosition(lngLatSE, lngLatNW, padding);
    JniHelpers::cameraPositionToJava(env, cameraPositionOut, camera);
}

void NATIVE_METHOD(flyTo)(JNIEnv* env, jobject obj, jdouble lon, jdouble lat,
                          jfloat zoom, jfloat duration, jfloat speed) {
    auto* map = androidMapFromJava(env, obj);

    CameraPosition camera = map->getCameraPosition();
    camera.longitude = lon;
    camera.latitude = lat;
    camera.zoom = zoom;
    map->flyTo(camera, duration, speed);
}

void NATIVE_METHOD(cancelCameraAnimation)(JNIEnv* env, jobject obj) {
    auto* map = androidMapFromJava(env, obj);
    map->cancelCameraAnimation();
}

jboolean NATIVE_METHOD(screenPositionToLngLat)(JNIEnv* env, jobject obj,
                                               jfloat x, jfloat y, jobject lngLatOut) {
    auto* map = androidMapFromJava(env, obj);

    LngLat lngLat{};
    bool result = map->screenPositionToLngLat(x, y, &lngLat.longitude, &lngLat.latitude);
    JniHelpers::lngLatToJava(env, lngLatOut, lngLat);
    return static_cast<jboolean>(result);
}

jboolean NATIVE_METHOD(lngLatToScreenPosition)(JNIEnv* env, jobject obj,
                                               jdouble lng, jdouble lat, jobject screenPositionOut, jboolean clipToViewport) {
    auto* map = androidMapFromJava(env, obj);

    double x = 0, y = 0;
    bool result = map->lngLatToScreenPosition(lng, lat, &x, &y, clipToViewport);
    JniHelpers::vec2ToJava(env, screenPositionOut, static_cast<float>(x), static_cast<float>(y));
    return static_cast<jboolean>(result);
}

void NATIVE_METHOD(setPixelScale)(JNIEnv* env, jobject obj, jfloat scale) {
    auto* map = androidMapFromJava(env, obj);
    map->setPixelScale(scale);
}

void NATIVE_METHOD(setCameraType)(JNIEnv* env, jobject obj, jint type) {
    auto* map = androidMapFromJava(env, obj);
    map->setCameraType(type);
}

jint NATIVE_METHOD(getCameraType)(JNIEnv* env, jobject obj) {
    auto* map = androidMapFromJava(env, obj);
    return map->getCameraType();
}

jfloat NATIVE_METHOD(getMinZoom)(JNIEnv* env, jobject obj) {
    auto* map = androidMapFromJava(env, obj);
    return map->getMinZoom();
}

void NATIVE_METHOD(setMinZoom)(JNIEnv* env, jobject obj, jfloat minZoom) {
    auto* map = androidMapFromJava(env, obj);
    map->setMinZoom(minZoom);
}

jfloat NATIVE_METHOD(getMaxZoom)(JNIEnv* env, jobject obj) {
    auto* map = androidMapFromJava(env, obj);
    return map->getMaxZoom();
}

void NATIVE_METHOD(setMaxZoom)(JNIEnv* env, jobject obj, jfloat maxZoom) {
    auto* map = androidMapFromJava(env, obj);
    map->setMaxZoom(maxZoom);
}

void NATIVE_METHOD(handleTapGesture)(JNIEnv* env, jobject obj, jfloat posX, jfloat posY) {
    auto* map = androidMapFromJava(env, obj);
    map->handleTapGesture(posX, posY);
}

void NATIVE_METHOD(handleDoubleTapGesture)(JNIEnv* env, jobject obj, jfloat posX, jfloat posY) {
    auto* map = androidMapFromJava(env, obj);
    map->handleDoubleTapGesture(posX, posY);
}

void NATIVE_METHOD(handlePanGesture)(JNIEnv* env, jobject obj, jfloat startX, jfloat startY,
                                     jfloat endX, jfloat endY) {
    auto* map = androidMapFromJava(env, obj);
    map->handlePanGesture(startX, startY, endX, endY);
}

void NATIVE_METHOD(handleFlingGesture)(JNIEnv* env, jobject obj, jfloat posX, jfloat posY,
                                       jfloat velocityX, jfloat velocityY) {
    auto* map = androidMapFromJava(env, obj);
    map->handleFlingGesture(posX, posY, velocityX, velocityY);
}

void NATIVE_METHOD(handlePinchGesture)(JNIEnv* env, jobject obj, jfloat posX, jfloat posY,
                                       jfloat scale, jfloat velocity) {
    auto* map = androidMapFromJava(env, obj);
    map->handlePinchGesture(posX, posY, scale, velocity);
}

void NATIVE_METHOD(handleRotateGesture)(JNIEnv* env, jobject obj, jfloat posX, jfloat posY,
                                        jfloat rotation) {
    auto* map = androidMapFromJava(env, obj);
    map->handleRotateGesture(posX, posY, rotation);
}

void NATIVE_METHOD(handleShoveGesture)(JNIEnv* env, jobject obj, jfloat distance) {
    auto* map = androidMapFromJava(env, obj);
    map->handleShoveGesture(distance);
}

void NATIVE_METHOD(onUrlComplete)(JNIEnv* env, jobject obj, jlong requestHandle,
                                  jbyteArray fetchedBytes, jstring errorString) {
    auto* map = androidMapFromJava(env, obj);
    map->androidPlatform().onUrlComplete(env, requestHandle, fetchedBytes, errorString);
}

void NATIVE_METHOD(setPickRadius)(JNIEnv* env, jobject obj, jfloat radius) {
    auto* map = androidMapFromJava(env, obj);
    map->setPickRadius(radius);
}

void NATIVE_METHOD(pickFeature)(JNIEnv* env, jobject obj, jfloat posX, jfloat posY) {
    auto* map = androidMapFromJava(env, obj);
    map->pickFeature(posX, posY);

}

void NATIVE_METHOD(pickMarker)(JNIEnv* env, jobject obj, jfloat posX, jfloat posY) {
    auto* map = androidMapFromJava(env, obj);
    map->pickMarker(posX, posY);
}

void NATIVE_METHOD(pickLabel)(JNIEnv* env, jobject obj, jfloat posX, jfloat posY) {
    auto* map = androidMapFromJava(env, obj);
    map->pickLabel(posX, posY);
}

jlong NATIVE_METHOD(markerAdd)(JNIEnv* env, jobject obj) {
    auto* map = androidMapFromJava(env, obj);
    auto markerID = map->markerAdd();
    // unsigned int to jlong for precision... else we can do jint return
    return static_cast<jlong>(markerID);
}

jboolean NATIVE_METHOD(markerRemove)(JNIEnv* env, jobject obj, jlong markerID) {
    auto* map = androidMapFromJava(env, obj);
    auto result = map->markerRemove(static_cast<unsigned int>(markerID));
    return static_cast<jboolean>(result);
}

jboolean NATIVE_METHOD(markerSetStylingFromString)(JNIEnv* env, jobject obj, jlong markerID,
                                                   jstring styling) {
    auto* map = androidMapFromJava(env, obj);

    auto styleString = JniHelpers::stringFromJavaString(env, styling);
    auto result = map->markerSetStylingFromString(static_cast<unsigned int>(markerID), styleString.c_str());
    return static_cast<jboolean>(result);
}

jboolean NATIVE_METHOD(markerSetStylingFromPath)(JNIEnv* env, jobject obj, jlong markerID,
                                                 jstring path) {
    auto* map = androidMapFromJava(env, obj);

    auto pathString = JniHelpers::stringFromJavaString(env, path);
    auto result = map->markerSetStylingFromPath(static_cast<unsigned int>(markerID), pathString.c_str());
    return static_cast<jboolean>(result);
}

jboolean NATIVE_METHOD(markerSetBitmap)(JNIEnv* env, jobject obj, jlong markerID,
                                        jobject jbitmap, jfloat density) {
    auto* map = androidMapFromJava(env, obj);

    AndroidBitmapInfo bitmapInfo;
    if (AndroidBitmap_getInfo(env, jbitmap, &bitmapInfo) != ANDROID_BITMAP_RESULT_SUCCESS) {
        return static_cast<jboolean>(false);
    }
    if (bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        // TODO: Add different conversion functions for other formats.
        return static_cast<jboolean>(false);
    }
    uint32_t* pixelInput;
    if (AndroidBitmap_lockPixels(env, jbitmap, (void**)&pixelInput) != ANDROID_BITMAP_RESULT_SUCCESS) {
        return static_cast<jboolean>(false);
    }
    int width = bitmapInfo.width;
    int height = bitmapInfo.height;
    uint32_t* pixelOutput = new uint32_t[height * width];
    int i = 0;
    for (int row = 0; row < height; row++) {
        // Flips image upside-down
        int flippedRow = (height - 1 - row) * width;
        for (int col = 0; col < width; col++) {
            uint32_t pixel = pixelInput[i++];
            // Undo alpha pre-multiplication.
            auto rgba = reinterpret_cast<uint8_t*>(&pixel);
            int a = rgba[3];
            if (a != 0) {
                auto alphaInv = 255.f/a;
                rgba[0] = static_cast<uint8_t>(rgba[0] * alphaInv);
                rgba[1] = static_cast<uint8_t>(rgba[1] * alphaInv);
                rgba[2] = static_cast<uint8_t>(rgba[2] * alphaInv);
            }
            pixelOutput[flippedRow + col] = pixel;
        }
    }
    AndroidBitmap_unlockPixels(env, jbitmap);
    auto result = map->markerSetBitmap(static_cast<unsigned int>(markerID), width, height, pixelOutput, density);
    delete[] pixelOutput;
    return static_cast<jboolean>(result);
}

jboolean NATIVE_METHOD(markerSetPoint)(JNIEnv* env, jobject obj, jlong markerID,
                                       jdouble lng, jdouble lat) {
    auto* map = androidMapFromJava(env, obj);

    auto result = map->markerSetPoint(static_cast<unsigned int>(markerID), Tangram::LngLat(lng, lat));
    return static_cast<jboolean>(result);
}

jboolean NATIVE_METHOD(markerSetPointEased)(JNIEnv* env, jobject obj, jlong markerID,
                                            jdouble lng, jdouble lat, jfloat duration, jint ease) {
    auto* map = androidMapFromJava(env, obj);

    auto result = map->markerSetPointEased(static_cast<unsigned int>(markerID),
                                           Tangram::LngLat(lng, lat), duration,
                                           static_cast<Tangram::EaseType>(ease));
    return static_cast<jboolean>(result);
}

jboolean NATIVE_METHOD(markerSetPolyline)(JNIEnv* env, jobject obj, jlong markerID,
                                          jdoubleArray jcoordinates, jint count) {
    auto* map = androidMapFromJava(env, obj);

    if (!jcoordinates || count == 0) { return static_cast<jboolean>(false); }

    auto* coordinates = env->GetDoubleArrayElements(jcoordinates, nullptr);
    std::vector<Tangram::LngLat> polyline;
    polyline.reserve(static_cast<size_t>(count));

    for (size_t i = 0; i < count; ++i) {
        polyline.emplace_back(coordinates[2 * i], coordinates[2 * i + 1]);
    }

    auto result = map->markerSetPolyline(static_cast<unsigned int>(markerID), polyline.data(), count);
    return static_cast<jboolean>(result);
}

jboolean NATIVE_METHOD(markerSetPolygon)(JNIEnv* env, jobject obj, jlong markerID,
                                         jdoubleArray jcoordinates, jintArray jcounts, jint rings) {
    auto* map = androidMapFromJava(env, obj);

    if (!jcoordinates || !jcounts || rings == 0) { return static_cast<jboolean>(false); }

    auto* coordinates = env->GetDoubleArrayElements(jcoordinates, nullptr);
    auto* counts = env->GetIntArrayElements(jcounts, nullptr);

    std::vector<Tangram::LngLat> polygonCoords;

    int coordsCount = 0;
    for (int i = 0; i < rings; i++) {
        int ringCount = *(counts+i);
        for (int j = 0; j < ringCount; j++) {
            polygonCoords.emplace_back(coordinates[coordsCount + 2 * j],
                                       coordinates[coordsCount + 2 * j + 1]);
        }
        coordsCount += ringCount;
    }

    auto result = map->markerSetPolygon(static_cast<unsigned int>(markerID),
                                        polygonCoords.data(), counts, rings);

    return static_cast<jboolean>(result);
}

jboolean NATIVE_METHOD(markerSetVisible)(JNIEnv* env, jobject obj, jlong markerID,
                                         jboolean visible) {
    auto* map = androidMapFromJava(env, obj);

    auto result = map->markerSetVisible(static_cast<unsigned int>(markerID), visible);
    return static_cast<jboolean>(result);
}

jboolean NATIVE_METHOD(markerSetDrawOrder)(JNIEnv* env, jobject obj, jlong markerID,
                                           jint drawOrder) {
    auto* map = androidMapFromJava(env, obj);

    auto result = map->markerSetDrawOrder(markerID, drawOrder);
    return static_cast<jboolean>(result);
}

void NATIVE_METHOD(markerRemoveAll)(JNIEnv* env, jobject obj) {
    auto* map = androidMapFromJava(env, obj);
    map->markerRemoveAll();
}


void NATIVE_METHOD(setDebugFlag)(JNIEnv* env, jobject obj, jint flag, jboolean on) {
    Tangram::setDebugFlag(static_cast<Tangram::DebugFlags>(flag), on);
}

void NATIVE_METHOD(useCachedGlState)(JNIEnv* env, jobject obj, jboolean use) {
    auto* map = androidMapFromJava(env, obj);
    map->useCachedGlState(use);
}

void NATIVE_METHOD(setDefaultBackgroundColor)(JNIEnv* env, jobject obj,
                                              jfloat r, jfloat g, jfloat b) {
    auto* map = androidMapFromJava(env, obj);
    map->setDefaultBackgroundColor(r, g, b);
}

jlong NATIVE_METHOD(addClientDataSource)(JNIEnv* env, jobject obj,
                                         jstring name, jboolean generateCentroid) {
    auto* map = androidMapFromJava(env, obj);

    auto sourceName = JniHelpers::stringFromJavaString(env, name);
    auto source = std::make_shared<ClientDataSource>(map->getPlatform(),
                                                              sourceName, "",
                                                              generateCentroid);
    map->addTileSource(source);
    return reinterpret_cast<jlong>(source.get());
}

void NATIVE_METHOD(removeClientDataSource)(JNIEnv* env, jobject obj, jlong sourcePtr) {
    auto* map = androidMapFromJava(env, obj);
    auto* clientDataSource = reinterpret_cast<ClientDataSource*>(sourcePtr);
    map->removeTileSource(*clientDataSource);
}

void NATIVE_METHOD(addClientDataFeature)(JNIEnv* env, jobject obj, jlong javaSourcePtr, jdoubleArray javaCoordinates, jintArray javaRings,
                               jobjectArray javaProperties) {
    auto* source = reinterpret_cast<ClientDataSource*>(javaSourcePtr);

    int nPoints = env->GetArrayLength(javaCoordinates) / 2;
    int nRings = (javaRings == NULL) ? 0 : env->GetArrayLength(javaRings);
    int nProperties = (javaProperties == NULL) ? 0 : env->GetArrayLength(javaProperties) / 2;

    Properties properties;

    for (int i = 0; i < nProperties; ++i) {
        jstring javaKey = (jstring) (env->GetObjectArrayElement(javaProperties, 2 * i));
        jstring javaValue = (jstring) (env->GetObjectArrayElement(javaProperties, 2 * i + 1));
        auto key = JniHelpers::stringFromJavaString(env, javaKey);
        auto value = JniHelpers::stringFromJavaString(env, javaValue);
        properties.set(key, value);
        env->DeleteLocalRef(javaKey);
        env->DeleteLocalRef(javaValue);
    }

    auto* coordinates = env->GetDoubleArrayElements(javaCoordinates, NULL);

    if (nRings > 0) {
        // If rings are defined, this is a polygon feature.
        auto* rings = env->GetIntArrayElements(javaRings, NULL);
        ClientDataSource::PolygonBuilder builder;
        builder.beginPolygon(static_cast<size_t>(nRings));
        int offset = 0;
        for (int j = 0; j < nRings; j++) {
            int nPointsInRing = rings[j];
            builder.beginRing(static_cast<size_t>(nPointsInRing));
            for (size_t i = 0; i < nPointsInRing; i++) {
                builder.addPoint(LngLat(coordinates[2 * (offset + i)], coordinates[2 * (offset + i) + 1]));
            }
            offset += nPointsInRing;
        }
        source->addPolygonFeature(std::move(properties), std::move(builder));
        env->ReleaseIntArrayElements(javaRings, rings, JNI_ABORT);
    } else if (nPoints > 1) {
        // If no rings defined but multiple points, this is a polyline feature.
        ClientDataSource::PolylineBuilder builder;
        builder.beginPolyline(static_cast<size_t>(nPoints));
        for (size_t i = 0; i < nPoints; i++) {
            builder.addPoint(LngLat(coordinates[2 * i], coordinates[2 * i + 1]));
        }
        source->addPolylineFeature(std::move(properties), std::move(builder));
    } else {
        // This is a point feature.
        source->addPointFeature(std::move(properties), LngLat(coordinates[0], coordinates[1]));
    }

    env->ReleaseDoubleArrayElements(javaCoordinates, coordinates, JNI_ABORT);
}

void NATIVE_METHOD(addClientDataGeoJson)(JNIEnv* env, jobject obj, jlong javaSourcePtr, jstring javaGeoJson) {
    auto* source = reinterpret_cast<ClientDataSource*>(javaSourcePtr);
    auto data = JniHelpers::stringFromJavaString(env, javaGeoJson);
    source->addData(data);
}

void NATIVE_METHOD(generateClientDataTiles)(JNIEnv* env, jobject obj, jlong javaSourcePtr) {
    auto* source = reinterpret_cast<ClientDataSource*>(javaSourcePtr);
    source->generateTiles();
}

void NATIVE_METHOD(clearClientDataFeatures)(JNIEnv* env, jobject obj, jlong javaSourcePtr) {
    auto* source = reinterpret_cast<ClientDataSource*>(javaSourcePtr);
    source->clearFeatures();
}

} // extern "C"

} // namespace Tangram
