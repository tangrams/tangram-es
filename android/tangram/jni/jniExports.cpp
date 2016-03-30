#include "tangram.h"
#include "platform_android.h"
#include "data/clientGeoJsonSource.h"

#include <cassert>

extern "C" {

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setPosition(JNIEnv* jniEnv, jobject obj, jdouble lon, jdouble lat) {
        Tangram::setPosition(lon, lat);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setPositionEased(JNIEnv* jniEnv, jobject obj, jdouble lon, jdouble lat, jfloat duration, jint ease) {
        Tangram::setPosition(lon, lat, duration, static_cast<Tangram::EaseType>(ease));
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_getPosition(JNIEnv* jniEnv, jobject obj, jdoubleArray lonLat) {
        jdouble* arr = jniEnv->GetDoubleArrayElements(lonLat, NULL);
        Tangram::getPosition(arr[0], arr[1]);
        jniEnv->ReleaseDoubleArrayElements(lonLat, arr, 0);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setZoom(JNIEnv* jniEnv, jobject obj, jfloat zoom) {
        Tangram::setZoom(zoom);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setZoomEased(JNIEnv* jniEnv, jobject obj, jfloat zoom, jfloat duration, jint ease) {
        Tangram::setZoom(zoom, duration, static_cast<Tangram::EaseType>(ease));
    }

    JNIEXPORT jfloat JNICALL Java_com_mapzen_tangram_MapController_getZoom(JNIEnv* jniEnv, jobject obj) {
        return Tangram::getZoom();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setRotation(JNIEnv* jniEnv, jobject obj, jfloat radians) {
        Tangram::setRotation(radians);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setRotationEased(JNIEnv* jniEnv, jobject obj, jfloat radians, jfloat duration, jint ease) {
        Tangram::setRotation(radians, duration, static_cast<Tangram::EaseType>(ease));
    }

    JNIEXPORT jfloat JNICALL Java_com_mapzen_tangram_MapController_getRotation(JNIEnv* jniEnv, jobject obj) {
        return Tangram::getRotation();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setTilt(JNIEnv* jniEnv, jobject obj, jfloat radians) {
        Tangram::setTilt(radians);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setTiltEased(JNIEnv* jniEnv, jobject obj, jfloat radians, jfloat duration, jint ease) {
        Tangram::setTilt(radians, duration, static_cast<Tangram::EaseType>(ease));
    }

    JNIEXPORT jfloat JNICALL Java_com_mapzen_tangram_MapController_getTilt(JNIEnv* jniEnv, jobject obj) {
        return Tangram::getTilt();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_screenToWorldCoordinates(JNIEnv* jniEnv, jobject obj, jdoubleArray screenPos) {
        jdouble* arr = jniEnv->GetDoubleArrayElements(screenPos, NULL);
        Tangram::screenToWorldCoordinates(arr[0], arr[1]);
        jniEnv->ReleaseDoubleArrayElements(screenPos, arr, 0);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_init(JNIEnv* jniEnv, jobject obj, jobject tangramInstance, jobject assetManager, jstring stylePath) {
        setupJniEnv(jniEnv, tangramInstance, assetManager);
        const char* cStylePath = jniEnv->GetStringUTFChars(stylePath, NULL);
        Tangram::initialize(cStylePath);
        jniEnv->ReleaseStringUTFChars(stylePath, cStylePath);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_loadScene(JNIEnv* jniEnv, jobject obj, jstring path) {
        const char* cPath = jniEnv->GetStringUTFChars(path, NULL);
        Tangram::loadScene(cPath);
        jniEnv->ReleaseStringUTFChars(path, cPath);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_resize(JNIEnv* jniEnv, jobject obj, jint width, jint height) {
        Tangram::resize(width, height);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_update(JNIEnv* jniEnv, jobject obj, jfloat dt) {
        Tangram::update(dt);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_render(JNIEnv* jniEnv, jobject obj) {
        Tangram::render();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setupGL(JNIEnv* jniEnv, jobject obj) {
        Tangram::setupGL();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setPixelScale(JNIEnv* jniEnv, jobject obj, jfloat scale) {
        Tangram::setPixelScale(scale);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setCameraType(JNIEnv* jniEnv, jobject obj, jint cameraType) {
        Tangram::setCameraType(cameraType);
    }

    JNIEXPORT jint JNICALL Java_com_mapzen_tangram_MapController_getCameraType(JNIEnv* jniEnv, jobject obj) {
        return Tangram::getCameraType();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_handleTapGesture(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY) {
        Tangram::handleTapGesture(posX, posY);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_handleDoubleTapGesture(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY) {
        Tangram::handleDoubleTapGesture(posX, posY);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_handlePanGesture(JNIEnv* jniEnv, jobject obj, jfloat startX, jfloat startY, jfloat endX, jfloat endY) {
        Tangram::handlePanGesture(startX, startY, endX, endY);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_handleFlingGesture(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY, jfloat velocityX, jfloat velocityY) {
        Tangram::handleFlingGesture(posX, posY, velocityX, velocityY);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_handlePinchGesture(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY, jfloat scale, jfloat velocity) {
        Tangram::handlePinchGesture(posX, posY, scale, velocity);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_handleRotateGesture(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY, jfloat rotation) {
        Tangram::handleRotateGesture(posX, posY, rotation);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_handleShoveGesture(JNIEnv* jniEnv, jobject obj, jfloat distance) {
        Tangram::handleShoveGesture(distance);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_onUrlSuccess(JNIEnv* jniEnv, jobject obj, jbyteArray fetchedBytes, jlong callbackPtr) {
        onUrlSuccess(jniEnv, fetchedBytes, callbackPtr);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_onUrlFailure(JNIEnv* jniEnv, jobject obj, jlong callbackPtr) {
        onUrlFailure(jniEnv, callbackPtr);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_pickFeature(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY) {
        auto& items = Tangram::pickFeaturesAt(posX, posY);
        if (!items.empty()) {
            featureSelectionCallback(jniEnv, items);
        }
    }

    JNIEXPORT jlong JNICALL Java_com_mapzen_tangram_MapController_nativeAddDataSource(JNIEnv* jniEnv, jobject obj, jstring name) {
        auto source_name = stringFromJString(jniEnv, name);
        auto source = std::shared_ptr<Tangram::DataSource>(new Tangram::ClientGeoJsonSource(source_name, ""));
        Tangram::addDataSource(source);
        return reinterpret_cast<jlong>(source.get());
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeRemoveDataSource(JNIEnv* jniEnv, jobject obj, jlong pointer) {
        assert(pointer > 0);
        auto source_ptr = reinterpret_cast<Tangram::DataSource*>(pointer);
        Tangram::removeDataSource(*source_ptr);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeClearDataSource(JNIEnv* jniEnv, jobject obj, jlong pointer) {
        assert(pointer > 0);
        auto source_ptr = reinterpret_cast<Tangram::DataSource*>(pointer);
        Tangram::clearDataSource(*source_ptr, true, true);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeAddFeature(JNIEnv* jniEnv, jobject obj, jlong pointer,
        jdoubleArray jcoordinates, jintArray jrings, jobjectArray jproperties) {

        assert(pointer > 0);
        auto source_ptr = reinterpret_cast<Tangram::ClientGeoJsonSource*>(pointer);

        size_t n_points = jniEnv->GetArrayLength(jcoordinates) / 2;
        size_t n_rings = (jrings == NULL) ? 0 : jniEnv->GetArrayLength(jrings);
        size_t n_properties = (jproperties == NULL) ? 0 : jniEnv->GetArrayLength(jproperties) / 2;

        Tangram::Properties properties;

        for (size_t i = 0; i < n_properties; ++i) {
            jstring jkey = (jstring) (jniEnv->GetObjectArrayElement(jproperties, 2 * i));
            jstring jvalue = (jstring) (jniEnv->GetObjectArrayElement(jproperties, 2 * i + 1));
            auto key = stringFromJString(jniEnv, jkey);
            auto value = stringFromJString(jniEnv, jvalue);
            properties.set(key, value);
            jniEnv->DeleteLocalRef(jkey);
            jniEnv->DeleteLocalRef(jvalue);
        }

        auto* coordinates = jniEnv->GetDoubleArrayElements(jcoordinates, NULL);

        if (n_rings > 0) {
            // If rings are defined, this is a polygon feature.
            auto* rings = jniEnv->GetIntArrayElements(jrings, NULL);
            std::vector<std::vector<Tangram::LngLat>> polygon;
            size_t ring_start = 0, ring_end = 0;
            for (size_t i = 0; i < n_rings; ++i) {
                ring_end += rings[i];
                std::vector<Tangram::LngLat> ring;
                for (; ring_start < ring_end; ++ring_start) {
                    ring.push_back({coordinates[2 * ring_start], coordinates[2 * ring_start + 1]});
                }
                polygon.push_back(std::move(ring));
            }
            source_ptr->addPoly(properties, polygon);
            jniEnv->ReleaseIntArrayElements(jrings, rings, JNI_ABORT);
        } else if (n_points > 1) {
            // If no rings defined but multiple points, this is a polyline feature.
            std::vector<Tangram::LngLat> polyline;
            for (size_t i = 0; i < n_points; ++i) {
                polyline.push_back({coordinates[2 * i], coordinates[2 * i + 1]});
            }
            source_ptr->addLine(properties, polyline);
        } else {
            // This is a point feature.
            auto point = Tangram::LngLat(coordinates[0], coordinates[1]);
            source_ptr->addPoint(properties, point);
        }

        jniEnv->ReleaseDoubleArrayElements(jcoordinates, coordinates, JNI_ABORT);

    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeAddGeoJson(JNIEnv* jniEnv, jobject obj, jlong pointer, jstring geojson) {
        assert(pointer > 0);
        auto source_ptr = reinterpret_cast<Tangram::ClientGeoJsonSource*>(pointer);
        auto data = stringFromJString(jniEnv, geojson);
        source_ptr->addData(data);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetDebugFlag(JNIEnv* jniEnv, jobject obj, jint flag, jboolean on) {
        Tangram::setDebugFlag(static_cast<Tangram::DebugFlags>(flag), on);
    }

}
