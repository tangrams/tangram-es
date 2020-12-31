#include <jni.h>
#include <tangram.h>
#include "JniHelpers.h"

namespace Tangram {

ClientDataSource* clientDataSourceFromJava(JNIEnv* env, jobject nativeMapDataObject) {
    static jclass nativeMapClass = env->FindClass("com/mapzen/tangram/NativeMapData");
    static jfieldID nativePointerFID = env->GetFieldID(nativeMapClass, "nativePointer", "J");
    jlong nativePointer = env->GetLongField(nativeMapDataObject, nativePointerFID);
    assert(nativePointer != 0);
    return reinterpret_cast<ClientDataSource*>(nativePointer);
}

extern "C" {

#define NATIVE_METHOD(NAME) JNIEXPORT JNICALL Java_com_mapzen_tangram_NativeMapData_ ## NAME

void NATIVE_METHOD(addClientDataFeature)(JNIEnv* env, jobject obj, jdoubleArray javaCoordinates, jintArray javaRings,
                                         jobjectArray javaProperties) {
    auto* source = clientDataSourceFromJava(env, obj);

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

void NATIVE_METHOD(addClientDataGeoJson)(JNIEnv* env, jobject obj, jstring javaGeoJson) {
    auto* source = clientDataSourceFromJava(env, obj);
    auto data = JniHelpers::stringFromJavaString(env, javaGeoJson);
    source->addData(data);
}

void NATIVE_METHOD(generateClientDataTiles)(JNIEnv* env, jobject obj) {
    auto* source = clientDataSourceFromJava(env, obj);
    source->generateTiles();
}

void NATIVE_METHOD(clearClientDataFeatures)(JNIEnv* env, jobject obj) {
    auto* source = clientDataSourceFromJava(env, obj);
    source->clearFeatures();
}

} // extern "C"

} // namespace Tangram
