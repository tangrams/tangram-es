package com.tangrams.reactnative;


import android.graphics.Bitmap;

import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.PointF;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;

import android.support.annotation.Nullable;
import android.util.Base64;
import android.util.Log;
import android.util.Xml;
import android.view.View;

import com.facebook.infer.annotation.Assertions;

import com.facebook.react.bridge.Arguments;
import com.facebook.react.bridge.JSApplicationIllegalArgumentException;

import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReadableArray;
import com.facebook.react.bridge.ReadableMap;
import com.facebook.react.bridge.ReadableMapKeySetIterator;
import com.facebook.react.bridge.WritableArray;
import com.facebook.react.bridge.WritableMap;

import com.facebook.react.common.MapBuilder;
import com.facebook.react.modules.core.DeviceEventManagerModule;
import com.facebook.react.modules.core.RCTNativeAppEventEmitter;
import com.facebook.react.uimanager.ThemedReactContext;
import com.facebook.react.uimanager.ViewGroupManager;
import com.facebook.react.uimanager.annotations.ReactProp;
import com.facebook.react.uimanager.events.RCTEventEmitter;

import com.mapzen.tangram.LngLat;
import com.mapzen.tangram.MapController;
import com.mapzen.tangram.MapData;
import com.mapzen.tangram.Marker;
import com.mapzen.tangram.SceneUpdate;
import com.mapzen.tangram.TouchInput;
import com.mapzen.tangram.geometry.Polygon;
import com.mapzen.tangram.geometry.Polyline;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Created by saeed tabrizi saeed@nowcando.com on 2/26/17.
 */

public class TangramMapViewManager extends ViewGroupManager<ReactTangramMapView> {

    private ReactApplicationContext _context;

    //private  new LngLat(51.00976419448854, 35.70532700869127); // init as Tehran

    private static final String TAG = "TangramMapViewManager";

    public TangramMapViewManager(ReactApplicationContext context) {
        super();
        _context = context;


    }

    public static final String REACT_CLASS = "RCTTangramMapView";

    @Override
    public String getName() {
        return REACT_CLASS;
    }

    public ReactApplicationContext getContext() {
        return _context;
    }


    // Lifecycle methods


    @Override
    public ReactTangramMapView createViewInstance(ThemedReactContext context) {

        return new ReactTangramMapView(context, _context, this);

    }


    @Override
    protected void onAfterUpdateTransaction(ReactTangramMapView view) {
        super.onAfterUpdateTransaction(view);

        view.onAfterUpdateTransaction();
    }

    /*@Override
    public void onDropViewInstance(ReactTangramMapView view) {

        view.onDrop();
    }*/

    // Event types

    public
    @Nullable
    Map<String, Object> getExportedCustomDirectEventTypeConstants() {


        return MapBuilder.<String, Object>builder()
                .put("onSceneReady", MapBuilder.of("registrationName", "onSceneReady"))
                .put("onFeaturePick", MapBuilder.of("registrationName", "onFeaturePick"))
                .put("onLabelPick", MapBuilder.of("registrationName", "onLabelPick"))
                .put("onMarkerPick", MapBuilder.of("registrationName", "onMarkerPick"))
                .put("onDoubleTap", MapBuilder.of("registrationName", "onDoubleTap"))
                .put("onLongPress", MapBuilder.of("registrationName", "onLongPress"))
                .put("onSingleTapUp", MapBuilder.of("registrationName", "onSingleTapUp"))
                .put("onSingleTapConfirmed", MapBuilder.of("registrationName", "onSingleTapConfirmed"))
                .put("onViewComplete", MapBuilder.of("registrationName", "onViewComplete"))
                .put("onPan", MapBuilder.of("registrationName", "onPan"))
                .put("onFling", MapBuilder.of("registrationName", "onFling"))
                .put("onScale", MapBuilder.of("registrationName", "onScale"))
                .put("onRotate", MapBuilder.of("registrationName", "onRotate"))
                .put("onShove", MapBuilder.of("registrationName", "onShove"))
                /*.put("onRegionChange", MapBuilder.of("registrationName", "onRegionChange"))
                .put("onRegionChangeComplete", MapBuilder.of("registrationName", "onRegionChangeComplete"))*/
                .build();


    }

    // Props

    protected String[] propsToStringArray(ReadableArray value) {
        ArrayList<String> arrayList = new ArrayList<>();
        for (int i = 0; i < value.size(); i++) {
            arrayList.add(value.getString(1));
        }

        return (String[]) arrayList.toArray();
    }

    @ReactProp(name = "scenePath")
    public void setScenePath(ReactTangramMapView view, String value) {
        try {
            view.setScenePath(value);

        } catch (Exception ex) {
            emitMapError(ex.getMessage(), "map_set_scene_path_error");
        }

    }

    @ReactProp(name = "sceneUpdates")
    public void setSceneUpdates(ReactTangramMapView view, ReadableMap smap) {
        try {

            ArrayList<SceneUpdate> uSeneces ;
            if(smap != null){
                uSeneces =  new ArrayList<SceneUpdate>();

                ReadableMapKeySetIterator itr = smap.keySetIterator();
                while (itr.hasNextKey()) {
                    String key = itr.nextKey();
                    uSeneces.add(new SceneUpdate(key,smap.getString(key)));
                }
            }
            else{
                uSeneces = view.getSceneUpdates();
            }
            view.setSceneUpdates(uSeneces);

        } catch (Exception ex) {
            emitMapError(ex.getMessage(), "map_set_scene_updates_error");
        }

    }

    @ReactProp(name = "layers")
    public void setLayers(ReactTangramMapView view, ReadableArray array) {
        try {
            if (view.getMapController() == null) {
                return;
            }
            String[] layers = this.propsToStringArray(array);

            Log.d(TAG, "new layers:" + layers.length);

            for (String name : layers) {

                view.getMapController().addDataLayer(name);
            }
        } catch (Exception ex) {
            emitMapError(ex.getMessage(), "map_set_layers_error");

        }

    }

    @ReactProp(name = "geoPosition")
    public void setGeoPosition(ReactTangramMapView view, ReadableMap coord) {
        try {
            if (!view.hasMap()) {
                return;
            }
            double lat = coord.getDouble("latitude");
            double lon = coord.getDouble("longitude");

            LngLat position = new LngLat(lon, lat);
            view.setGeoPosition(position);

        } catch (Exception ex) {
            emitMapError(ex.getMessage(), "map_set_position_error");
        }

    }

    @ReactProp(name = "pickRadius", defaultFloat = .5f)
    public void setPickRadius(ReactTangramMapView view, double value) {
        try {
            if (!view.hasMap()) {
                return;
            }
            view.setPickRadius((float) value);
        } catch (Exception ex) {
            emitMapError(ex.getMessage(), "map_set_pick_radius_error");
        }

    }

    @ReactProp(name = "cacheName")
    public void setCacheName(ReactTangramMapView view, String value) {
        view.setCacheName(value);

    }

    @ReactProp(name = "cacheSize", defaultFloat = 0f)
    public void setCacheSize(ReactTangramMapView view, double value) {
        view.setCacheSize(Math.max((long) value, 0));

    }

    @ReactProp(name = "cameraType")
    public void setCameraType(ReactTangramMapView view, @Nullable String value) {
        try {
            if (view.getMapController() == null) {
                return;
            }
            if (value == null || value.isEmpty()) {
                value = "FLAT";
            }
            value = value.toUpperCase();
            MapController.CameraType cameraType = MapController.CameraType.valueOf(value);
            view.setCameraType(cameraType);

        } catch (Exception ex) {
            emitMapError(ex.getMessage(), "map_set_camera_type_error");
        }

    }

    @ReactProp(name = "zoom", defaultFloat = 10f)
    public void setZoom(ReactTangramMapView view, double value) {
        view.setZoom((float) value);
    }

    @ReactProp(name = "minZoom", defaultFloat = 1f)
    public void setMinZoom(ReactTangramMapView view, double value) {
        view.setMinZoom((float) value);

    }

    @ReactProp(name = "maxZoom", defaultFloat = 24f)
    public void setMaxZoom(ReactTangramMapView view, double value) {
        view.setMaxZoom((float) value);
    }


    @ReactProp(name = "tilt", defaultFloat = 0)
    public void setTilt(ReactTangramMapView view, double value) {
        view.setTilt((float) value);
    }

    @ReactProp(name = "minTilt")
    public void setMinTilt(ReactTangramMapView view, double value) {
        view.setMinTilt((float) value);
    }

    @ReactProp(name = "maxTilt")
    public void setMaxTilt(ReactTangramMapView view, double value) {
        view.setMaxTilt((float) value);
    }

    @ReactProp(name = "rotate", defaultFloat = 0)
    public void setRotate(ReactTangramMapView view, double value) {
        view.setRotate((float) value);
    }

    @ReactProp(name = "minRotate")
    public void setMinRotate(ReactTangramMapView view, double value) {
        view.setMinRotate((float) value);
    }

    @ReactProp(name = "maxRotate")
    public void setMaxRotate(ReactTangramMapView view, double value) {
        view.setMaxRotate((float) value);
    }

    @ReactProp(name = "handleDoubleTap")
    public void setHandleDoubleTap(ReactTangramMapView view, boolean enable) {
        view.setHandleDoubleTap( enable);
    }

    @ReactProp(name = "handleSingleTapUp")
    public void setHandleSingleTapUp(ReactTangramMapView view, boolean enable) {
        view.setHandleSingleTapUp( enable);
    }

    @ReactProp(name = "handleSingleTapConfirmed")
    public void setHandleSingleTapConfirmed(ReactTangramMapView view, boolean enable) {
        view.setHandleSingleTapConfirmed( enable);
    }

    @ReactProp(name = "handleFling")
    public void setHandleFling(ReactTangramMapView view, boolean enable) {
        view.setHandleFling( enable);
    }

    @ReactProp(name = "handlePan")
    public void setHandlePan(ReactTangramMapView view, boolean enable) {
        view.setHandlePan( enable);
    }



    @ReactProp(name = "handleScale")
    public void setHandleScale(ReactTangramMapView view, boolean enable) {
        view.setHandleScale( enable);
    }


    @ReactProp(name = "handleShove")
    public void setHandleShove(ReactTangramMapView view, boolean enable) {
        view.setHandleShove( enable);
    }

    @ReactProp(name = "handleRotate")
    public void setHandleRotate(ReactTangramMapView view, boolean enable) {
        view.setHandleRotate( enable);
    }

    // @ReactProp(name = "enableLoadingView")
    public void setEnableLoadingView(ReactTangramMapView view, boolean enable) {
        view.setHandleRotate( enable);
    }





    // Commands

    public static final int COMMAND_GET_GEO_POSITION = 0;
    public static final int COMMAND_GET_TILT = 1;
    public static final int COMMAND_GET_ROTATION = 2;
    public static final int COMMAND_GET_ZOOM = 3;
    public static final int COMMAND_GET_CAMERA_TYPE = 4;

    public static final int COMMAND_SET_GEO_POSITION = 5;
    public static final int COMMAND_SET_TILT = 6;
    public static final int COMMAND_SET_ROTATION = 7;
    public static final int COMMAND_SET_ZOOM = 8;
    public static final int COMMAND_SET_CAMERA_TYPE = 9;

    public static final int COMMAND_SET_POSITION_EASE = 10;
    public static final int COMMAND_SET_TILT_EASE = 11;
    public static final int COMMAND_SET_ROTATION_EASE = 12;
    public static final int COMMAND_SET_ZOOM_EASE = 13;

    public static final int COMMAND_SET_PICK_RADIUS = 14;
    public static final int COMMAND_PICK_LABEL = 15;
    public static final int COMMAND_PICK_FEATURE = 16;
    public static final int COMMAND_PICK_MARKER = 17;

    public static final int COMMAND_ADD_DATA_LAYER = 21;
    public static final int COMMAND_ADD_POINT_DATA_LAYER = 22;
    public static final int COMMAND_ADD_POLYGON_DATA_LAYER = 23;
    public static final int COMMAND_ADD_POLYLINE_DATA_LAYER = 24;
    public static final int COMMAND_CLEAR_DATA_LAYER = 25;
    public static final int COMMAND_REMOVE_DATA_LAYER = 26;

    public static final int COMMAND_ADD_MARKER = 51;
    public static final int COMMAND_PUT_MARKER = 52;

    public static final int COMMAND_UPDATE_MARKER = 53;

    public static final int COMMAND_REMOVE_MARKER = 54;
    public static final int COMMAND_REMOVE_ALL_MARKERS = 55;
    public static final int COMMAND_SET_MARKER_VISIBLE = 61;
    public static final int COMMAND_SET_MARKER_DRAW_ORDER = 62;
    public static final int COMMAND_SET_MARKER_POINT = 63;
    public static final int COMMAND_SET_MARKER_POINT_EASED = 64;
    public static final int COMMAND_SET_MARKER_POLYGON = 65;
    public static final int COMMAND_SET_MARKER_POLYLINE = 66;
    public static final int COMMAND_SET_MARKER_STYLE_FROM_STRING = 67;
    public static final int COMMAND_SET_MARKER_STYLE_FROM_PATH = 68;
    public static final int COMMAND_SET_MARKER_BITMAP = 69;

    public static final int COMMAND_UPDATE_SCENE_ASYNC = 81;
    public static final int COMMAND_LOAD_SCENE_FILE = 82;
    public static final int COMMAND_LOAD_SCENE_FILE_ASYNC = 83;
    public static final int COMMAND_REQUEST_RENDER = 84;
    public static final int COMMAND_IS_SIMULTANEOUSGESTUREALLOWED = 85;
    public static final int COMMAND_USE_CACHED_GLSTATE = 86;
    public static final int COMMAND_CAPTURE_FRAME = 87;

    public static final int COMMAND_SCREEN_TO_LNGLAT = 91;
    public static final int COMMAND_LNGLAT_TO_SCREEN = 92;


    @Override
    public
    @Nullable
    Map<String, Integer> getCommandsMap() {
        return MapBuilder.<String, Integer>builder()
                .put("getGeoPosition", COMMAND_GET_GEO_POSITION)
                .put("getRotation", COMMAND_GET_ROTATION)
                .put("getTilt", COMMAND_GET_TILT)
                .put("getZoom", COMMAND_GET_ZOOM)
                .put("getCameraType", COMMAND_GET_CAMERA_TYPE)

                .put("setGeoPosition", COMMAND_SET_GEO_POSITION)
                .put("setRotation", COMMAND_SET_ROTATION)
                .put("setTilt", COMMAND_SET_TILT)
                .put("setZoom", COMMAND_SET_ZOOM)
                .put("setCameraType", COMMAND_SET_CAMERA_TYPE)

                .put("setPositionEase", COMMAND_SET_POSITION_EASE)
                .put("setRotationEase", COMMAND_SET_ROTATION_EASE)
                .put("setTiltEase", COMMAND_SET_TILT_EASE)
                .put("setZoomEase", COMMAND_SET_ZOOM_EASE)

                .put("setPickRadius", COMMAND_SET_PICK_RADIUS)
                .put("pickFeature", COMMAND_PICK_FEATURE)
                .put("pickLabel", COMMAND_PICK_LABEL)
                .put("pickMarker", COMMAND_PICK_MARKER)


                .put("addDataLayer", COMMAND_ADD_DATA_LAYER)
                .put("addPointDataLayer", COMMAND_ADD_POINT_DATA_LAYER)
                .put("addPolygonDataLayer", COMMAND_ADD_POLYGON_DATA_LAYER)
                .put("addPolylineDataLayer", COMMAND_ADD_POLYLINE_DATA_LAYER)
                .put("removeDataLayer", COMMAND_REMOVE_DATA_LAYER)
                .put("clearDataLayer", COMMAND_CLEAR_DATA_LAYER)

                .put("addMarker", COMMAND_ADD_MARKER)
                .put("putMarker", COMMAND_PUT_MARKER)
                .put("updateMarker", COMMAND_UPDATE_MARKER)
                .put("removeAllMarkers", COMMAND_REMOVE_ALL_MARKERS)
                .put("removeMarker", COMMAND_REMOVE_MARKER)

                .put("setMarkerVisible", COMMAND_SET_MARKER_VISIBLE)
                .put("setMarkerDrawOrder", COMMAND_SET_MARKER_DRAW_ORDER)
                .put("setMarkerPoint", COMMAND_SET_MARKER_POINT)
                .put("setMarkerPointEased", COMMAND_SET_MARKER_POINT_EASED)
                .put("setMarkerPolygon", COMMAND_SET_MARKER_POLYGON)
                .put("setMarkerPolyline", COMMAND_SET_MARKER_POLYLINE)
                .put("setMarkerStylingFromPath", COMMAND_SET_MARKER_STYLE_FROM_PATH)
                .put("setMarkerStylingFromString", COMMAND_SET_MARKER_STYLE_FROM_STRING)
                .put("setMarkerBitmap", COMMAND_SET_MARKER_BITMAP)

                .put("requestRender", COMMAND_REQUEST_RENDER)
                .put("updateSceneAsync", COMMAND_UPDATE_SCENE_ASYNC)
                .put("loadSceneFile", COMMAND_LOAD_SCENE_FILE)
                .put("loadSceneFileAsync", COMMAND_LOAD_SCENE_FILE_ASYNC)
                .put("isSimultaneousGestureAllowed", COMMAND_IS_SIMULTANEOUSGESTUREALLOWED)
                .put("useCachedGlState", COMMAND_USE_CACHED_GLSTATE)
                .put("captureFrame", COMMAND_CAPTURE_FRAME)


                .put("screenToLngLat", COMMAND_SCREEN_TO_LNGLAT)
                .put("lnglatToScreen", COMMAND_LNGLAT_TO_SCREEN)
                .build();
    }

    private void emitMapError(String message, String type) {
        WritableMap error = Arguments.createMap();
        error.putString("message", message);
        error.putString("type", type);

        _context
                .getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter.class)
                .emit("onError", error);
    }

    private void fireSuccessCallback(int callbackId, WritableArray args) {
        WritableArray event = Arguments.createArray();
        event.pushInt(callbackId);
        event.pushArray(args);

        _context.getJSModule(RCTNativeAppEventEmitter.class)
                .emit("RCTTangramMapViewAndroidCallback", event);
    }

    private void fireErrorCallback(int callbackId, int code ,String message, String type) {
        WritableArray event = Arguments.createArray();
        WritableMap error = Arguments.createMap();
        error.putInt("code", code);
        error.putString("message", message);
        error.putString("type", type);

        event.pushInt(callbackId);
        event.pushMap(error);

        _context.getJSModule(RCTNativeAppEventEmitter.class)
                .emit("RCTTangramMapViewAndroidCallback", error);
    }

    @Override
    public void receiveCommand(ReactTangramMapView view, int commandId, @Nullable ReadableArray args) {
        Assertions.assertNotNull(args);

        LngLat point = null;
        Polygon polygon = null;
        Polyline polyline = null;
        Drawable drawable = null;
        Map<String, String> mpr = null;
        ReadableMap mps = null;
        List<List<LngLat>> listListpoints = null;
        List<LngLat> listpoints = null;
        ArrayList<SceneUpdate> uSeneces = null ;
        try {


            switch (commandId) {
                case COMMAND_GET_GEO_POSITION:
                    getGeoPosition(view, args.getInt(0) , args.getInt(1) );
                    break;
                case COMMAND_GET_TILT:
                    getTilt(view, args.getInt(0) , args.getInt(1) );
                    break;
                case COMMAND_GET_ZOOM:
                    getZoom(view, args.getInt(0) , args.getInt(1) );
                    break;
                case COMMAND_GET_ROTATION:
                    getRotation(view, args.getInt(0) , args.getInt(1) );
                    break;
                case COMMAND_GET_CAMERA_TYPE:
                    getCameraType(view, args.getInt(0) , args.getInt(1) );
                    break;

                case COMMAND_SET_GEO_POSITION:
                    setGeoPosition(view, args.getInt(0) , args.getInt(1) , new LngLat(args.getMap(2).getDouble("longitude"), args.getMap(2).getDouble("latitude")));
                    break;
                case COMMAND_SET_TILT:
                    setTilt(view, args.getInt(0) , args.getInt(1) , (float) args.getDouble(2));
                    break;
                case COMMAND_SET_ZOOM:
                    setZoom(view, args.getInt(0) , args.getInt(1) , (float) args.getDouble(2));
                    break;
                case COMMAND_SET_ROTATION:
                    setRotation(view, args.getInt(0) , args.getInt(1) , (float) args.getDouble(2));
                    break;
                case COMMAND_SET_CAMERA_TYPE:
                    setCameraType(view, args.getInt(0) , args.getInt(1) , MapController.CameraType.valueOf(args.getString(2)));
                    break;
                case COMMAND_SET_POSITION_EASE:
                    setPositionEase(view, args.getInt(0) , args.getInt(1) , new LngLat(args.getMap(2).getDouble("longitude"), args.getMap(2).getDouble("latitude")), args.getInt(3), MapController.EaseType.valueOf(args.getString(4).toUpperCase()));
                    break;
                case COMMAND_SET_TILT_EASE:
                    setTiltEase(view, args.getInt(0) , args.getInt(1) , (float) args.getDouble(2), args.getInt(3), MapController.EaseType.valueOf(args.getString(4).toUpperCase()));
                    break;
                case COMMAND_SET_ZOOM_EASE:
                    setZoomEase(view, args.getInt(0) , args.getInt(1) , (float) args.getDouble(2), args.getInt(3), MapController.EaseType.valueOf(args.getString(4).toUpperCase()));
                    break;
                case COMMAND_SET_ROTATION_EASE:
                    setRotationEase(view, args.getInt(0) , args.getInt(1) , (float) args.getDouble(2), args.getInt(3), MapController.EaseType.valueOf(args.getString(4).toUpperCase()));
                    break;

                case COMMAND_SET_PICK_RADIUS:
                    setPickRadius(view, args.getInt(0) , args.getInt(1) , (float) args.getDouble(2));
                    break;
                case COMMAND_PICK_FEATURE:
                    pickFeature(view, args.getInt(0) , args.getInt(1) , (float) args.getDouble(2), (float) args.getDouble(3));
                    break;
                case COMMAND_PICK_LABEL:
                    pickLabel(view, args.getInt(0) , args.getInt(1) , (float) args.getDouble(2), (float) args.getDouble(3));
                    break;
                case COMMAND_PICK_MARKER:
                    pickMarker(view, args.getInt(0) , args.getInt(1) , (float) args.getDouble(2), (float) args.getDouble(3));
                    break;


                case COMMAND_ADD_DATA_LAYER:
                    addDataLayer(view, args.getInt(0) , args.getInt(1) , args.getString(2), args.getString(3));
                    break;
                case COMMAND_ADD_POINT_DATA_LAYER:
                    mps = args.getMap(4);
                    if (mps != null) {
                        mpr = MapBuilder.newHashMap();
                        ReadableMapKeySetIterator itr = mps.keySetIterator();
                        while (itr.hasNextKey()) {
                            String key = itr.nextKey();
                            mpr.put(key, args.getMap(1).getString(key));
                        }
                    }
                    addPointToMapDataLayer(view, args.getInt(0) , args.getInt(1) , args.getString(2),
                            new LngLat(args.getMap(3).getDouble("longitude"),
                                    args.getMap(3).getDouble("latitude")),
                            mpr);
                    break;
                case COMMAND_ADD_POLYGON_DATA_LAYER:
                    mps = args.getMap(4);
                    if (mps != null) {
                        mpr = MapBuilder.newHashMap();
                        ReadableMapKeySetIterator itr = mps.keySetIterator();
                        while (itr.hasNextKey()) {
                            String key = itr.nextKey();
                            mpr.put(key, args.getMap(1).getString(key));
                        }
                    }
                    //TODO implement in next version
                    // addPolygonMapDataLayer(view, args.getInt(0) , args.getInt(1) , args.getString(1), args.getString(2));
                    break;
                case COMMAND_ADD_POLYLINE_DATA_LAYER:
                    //TODO implement in next version
                    //addPolylineMapDataLayer(view, args.getInt(0) , args.getInt(1) , args.getString(1), args.getString(2));
                    break;
                case COMMAND_REMOVE_DATA_LAYER:
                    removeMapDataLayer(view, args.getInt(0) , args.getInt(1) , args.getString(2));
                    break;
                case COMMAND_CLEAR_DATA_LAYER:
                    clearMapDataLayer(view, args.getInt(0) , args.getInt(1) , args.getString(2));
                    break;

                case COMMAND_ADD_MARKER:
                    addMapMarker(view, args.getInt(0) , args.getInt(1) );
                    break;
                case COMMAND_PUT_MARKER:
                    if (!args.isNull(4)) {
                        point = new LngLat((float) args.getMap(4).getDouble("longitude"), (float) args.getMap(4).getDouble("latitude"));

                    }
                    if (!args.isNull(5)) {
                        mps = args.getMap(5);
                        listListpoints = new ArrayList<>();


                        for (int i = 0; i < mps.getArray("points").size(); i++) {
                            listpoints = new ArrayList<>();
                            for (int j = 0; j < mps.getArray("points").getArray(i).size(); j++) {
                                ReadableMap rmp = mps.getArray("points").getArray(i).getMap(0);
                                listpoints.add(new LngLat(rmp.getDouble("longitude"), rmp.getDouble("latitude")));
                            }
                            listListpoints.add(listpoints);
                        }

                        if (mps.getMap("properties") != null) {
                            mpr = MapBuilder.newHashMap();
                            ReadableMapKeySetIterator itr = mps.getMap("properties").keySetIterator();
                            while (itr.hasNextKey()) {
                                String key = itr.nextKey();
                                mpr.put(key, args.getMap(1).getString(key));
                            }
                        }


                        polygon = new Polygon(listListpoints, mpr);
                    }
                    if (!args.isNull(6)) {
                        mps = args.getMap(6);
                        listpoints = new ArrayList<>();
                        mpr = null;
                        for (int i = 0; i < mps.getArray("points").size(); i++) {
                            ReadableMap rmp = mps.getArray("points").getMap(0);
                            listpoints.add(new LngLat(rmp.getDouble("longitude"), rmp.getDouble("latitude")));
                        }
                        if (mps.getMap("properties") != null) {
                            mpr = MapBuilder.newHashMap();
                            ReadableMapKeySetIterator itr = mps.getMap("properties").keySetIterator();
                            while (itr.hasNextKey()) {
                                String key = itr.nextKey();
                                mpr.put(key, args.getMap(1).getString(key));
                            }
                        }
                        polyline = new Polyline(listpoints,mpr);
                    }
                    if (!args.isNull(8)) {
                        //TODO : implement in next release
                        //drawable = DrawableWrapper.createFromStream()
                    }

                    putMapMarker(view, args.getInt(0) , args.getInt(1) , args.getBoolean(2),
                            args.isNull(3) ? null : args.getInt(3), point, polygon, polyline,
                            args.isNull(7) ? null : args.getString(7),
                            args.isNull(8) ? null : args.getInt(8), drawable);
                    break;
                case COMMAND_UPDATE_MARKER:

                    if (!args.isNull(5) && args.getMap(5) != null) {
                        point = new LngLat((float) args.getMap(5).getDouble("longitude"), (float) args.getMap(5).getDouble("latitude"));
                    }
                    if (!args.isNull(6) && args.getMap(6) != null) {
                        mps = args.getMap(6);
                        listListpoints = new ArrayList<>();
                        mpr = null;

                        for (int i = 0; i < mps.getArray("points").size(); i++) {
                            listpoints = new ArrayList<>();
                            for (int j = 0; j < mps.getArray("points").getArray(i).size(); j++) {
                                ReadableMap rmp = mps.getArray("points").getArray(i).getMap(0);
                                listpoints.add(new LngLat(rmp.getDouble("longitude"), rmp.getDouble("latitude")));
                            }
                            listListpoints.add(listpoints);
                        }

                        if (mps.getMap("properties") != null) {
                            mpr = MapBuilder.newHashMap();
                            ReadableMapKeySetIterator itr = mps.getMap("properties").keySetIterator();
                            while (itr.hasNextKey()) {
                                String key = itr.nextKey();
                                mpr.put(key, args.getMap(1).getString(key));
                            }
                        }


                        polygon = new Polygon(listListpoints, mpr);
                    }
                    if (!args.isNull(7) && args.getMap(7) != null) {
                        mps = args.getMap(7);
                        listpoints = new ArrayList<>();
                        mpr = null;
                        for (int i = 0; i < mps.getArray("points").size(); i++) {
                            ReadableMap rmp = mps.getArray("points").getMap(0);
                            listpoints.add(new LngLat(rmp.getDouble("longitude"), rmp.getDouble("latitude")));
                        }
                        if (mps.getMap("properties") != null) {
                            mpr = MapBuilder.newHashMap();
                            ReadableMapKeySetIterator itr = mps.getMap("properties").keySetIterator();
                            while (itr.hasNextKey()) {
                                String key = itr.nextKey();
                                mpr.put(key, args.getMap(1).getString(key));
                            }
                        }
                        polyline = new Polyline(listpoints,mpr);
                    }
                    if (!args.isNull(10) && args.getArray(10) != null) {
                        //TODO : implement in next release
                        //drawable = DrawableWrapper.createFromStream()
                    }
                    updateMapMarker(view, args.getInt(0) , args.getInt(1) , (long) args.getDouble(2), args.getBoolean(3),
                            !args.isNull(4)  ? args.getInt(4) : null, point, polygon, polyline,
                            !args.isNull(8)  ? args.getString(8) : null, !args.isNull(9)  ? args.getInt(9) : null, drawable);
                    break;

                case COMMAND_REMOVE_ALL_MARKERS:
                    removeAllMarkers(view, args.getInt(0) , args.getInt(1) );
                    break;

                case COMMAND_REMOVE_MARKER:
                    removeMarker(view, args.getInt(0) , args.getInt(1) , (long) args.getDouble(2));
                    break;

                case COMMAND_SET_MARKER_VISIBLE:
                    setMarkerVisible(view, args.getInt(0) , args.getInt(1) , (long)args.getDouble(2) ,args.getBoolean(3) );
                    break;
                case COMMAND_SET_MARKER_DRAW_ORDER:
                    setMarkerDrawOrder(view, args.getInt(0) , args.getInt(1) , (long)args.getDouble(2) ,args.getInt(3) );
                    break;
                case COMMAND_SET_MARKER_POINT:
                    if (!args.isNull(3) && args.getMap(3) != null) {
                        point = new LngLat((float) args.getMap(3).getDouble("longitude"), (float) args.getMap(3).getDouble("latitude"));
                    }
                    setMarkerPoint(view, args.getInt(0) , args.getInt(1) , (long)args.getDouble(2) , point );
                    break;
                case COMMAND_SET_MARKER_POINT_EASED:
                    if (!args.isNull(3) && args.getMap(3) != null) {
                        point = new LngLat((float) args.getMap(3).getDouble("longitude"), (float) args.getMap(3).getDouble("latitude"));
                    }
                    setMarkerPointEased(view, args.getInt(0) , args.getInt(1) , (long)args.getDouble(2) , point , args.getInt(4) , MapController.EaseType.valueOf(args.getString(5).toUpperCase()) );
                    break;
                case COMMAND_SET_MARKER_POLYGON:
                    if (!args.isNull(3) && args.getMap(3) != null) {
                        mps = args.getMap(3);
                        listListpoints = new ArrayList<>();
                        mpr = null;

                        for (int i = 0; i < mps.getArray("points").size(); i++) {
                            listpoints = new ArrayList<>();
                            for (int j = 0; j < mps.getArray("points").getArray(i).size(); j++) {
                                ReadableMap rmp = mps.getArray("points").getArray(i).getMap(0);
                                listpoints.add(new LngLat(rmp.getDouble("longitude"), rmp.getDouble("latitude")));
                            }
                            listListpoints.add(listpoints);
                        }

                        if (mps.getMap("properties") != null) {
                            mpr = MapBuilder.newHashMap();
                            ReadableMapKeySetIterator itr = mps.getMap("properties").keySetIterator();
                            while (itr.hasNextKey()) {
                                String key = itr.nextKey();
                                mpr.put(key, args.getMap(1).getString(key));
                            }
                        }


                        polygon = new Polygon(listListpoints, mpr);
                    }
                    setMarkerPolygon(view, args.getInt(0) , args.getInt(1) , (long)args.getDouble(2) , polygon);
                    break;
                case COMMAND_SET_MARKER_POLYLINE:
                    if (!args.isNull(3) && args.getMap(3) != null) {
                        mps = args.getMap(3);
                        listpoints = new ArrayList<>();
                        mpr = null;
                        for (int i = 0; i < mps.getArray("points").size(); i++) {
                            ReadableMap rmp = mps.getArray("points").getMap(0);
                            listpoints.add(new LngLat(rmp.getDouble("longitude"), rmp.getDouble("latitude")));
                        }
                        if (mps.getMap("properties") != null) {
                            mpr = MapBuilder.newHashMap();
                            ReadableMapKeySetIterator itr = mps.getMap("properties").keySetIterator();
                            while (itr.hasNextKey()) {
                                String key = itr.nextKey();
                                mpr.put(key, args.getMap(1).getString(key));
                            }
                        }
                        polyline = new Polyline(listpoints,mpr);
                    }
                    setMarkerPolyline(view, args.getInt(0) , args.getInt(1) , (long)args.getDouble(2) , polyline );
                    break;
                case COMMAND_SET_MARKER_STYLE_FROM_PATH:
                    setMarkerStylingFromPath(view, args.getInt(0) , args.getInt(1) , (long)args.getDouble(2) ,args.getString(3) );
                    break;
                case COMMAND_SET_MARKER_STYLE_FROM_STRING:
                    setMarkerStylingFromString(view, args.getInt(0) , args.getInt(1) , (long)args.getDouble(2) ,args.getString(3) );
                    break;
                case COMMAND_SET_MARKER_BITMAP:
                    setMarkerBitmap(view, args.getInt(0) , args.getInt(1) , (long)args.getDouble(2) ,args.getString(3) );
                    break;
                case COMMAND_REQUEST_RENDER:
                    requestRender(view, args.getInt(0) , args.getInt(1) );
                    break;
                case COMMAND_UPDATE_SCENE_ASYNC:

                    if(!args.isNull(2) && args.getMap(2) != null){
                        uSeneces =  new ArrayList<>();
                        ReadableMap usm =  args.getMap(2);
                        ReadableMapKeySetIterator itr = usm.keySetIterator();
                        while (itr.hasNextKey()) {
                            String key = itr.nextKey();
                            uSeneces.add(new SceneUpdate(key,usm.getString(key)));
                        }
                    }
                    updateSceneAsync(view, args.getInt(0) , args.getInt(1), uSeneces );
                    break;
                case COMMAND_LOAD_SCENE_FILE:

                    if(!args.isNull(3) && args.getMap(3) != null){
                        uSeneces =  new ArrayList<>();
                        ReadableMap usm =  args.getMap(3);
                        ReadableMapKeySetIterator itr = usm.keySetIterator();
                        while (itr.hasNextKey()) {
                            String key = itr.nextKey();
                            uSeneces.add(new SceneUpdate(key,usm.getString(key)));
                        }
                    }
                    else{
                        uSeneces = view.getSceneUpdates();
                    }
                    loadSceneFile(view, args.getInt(0) , args.getInt(1) , args.getString(2), uSeneces);
                    break;
                case COMMAND_LOAD_SCENE_FILE_ASYNC:
                    if(!args.isNull(3) && args.getMap(3) != null){
                        uSeneces =  new ArrayList<>();
                        ReadableMap usm =  args.getMap(3);
                        ReadableMapKeySetIterator itr = usm.keySetIterator();
                        while (itr.hasNextKey()) {
                            String key = itr.nextKey();
                            uSeneces.add(new SceneUpdate(key,usm.getString(key)));
                        }
                    }
                    else{
                        uSeneces = view.getSceneUpdates();
                    }

                    loadSceneFileAsync(view, args.getInt(0) , args.getInt(1) , args.getString(2), uSeneces);
                    break;
                case COMMAND_IS_SIMULTANEOUSGESTUREALLOWED:
                    isSimultaneousGestureAllowed(view, args.getInt(0) , args.getInt(1) , TouchInput.Gestures.valueOf(args.getString(1).toUpperCase()), TouchInput.Gestures.valueOf(args.getString(2).toUpperCase()));
                    break;
                case COMMAND_CAPTURE_FRAME:
                    captureFrame(view, args.getInt(0) , args.getInt(1) , args.getBoolean(2),
                            args.isNull(3) ? null:  args.getInt(3),
                            args.isNull(4) ? null:  args.getInt(4),
                            args.isNull(5) ? null:  args.getInt(5));
                    break;
                case COMMAND_USE_CACHED_GLSTATE:
                    useCachedGlState(view, args.getInt(0) , args.getInt(1) , args.getBoolean(2));
                    break;

                case COMMAND_SCREEN_TO_LNGLAT:
                    ReadableMap rmp1 = args.getMap(2);
                    screenToLngLat(view, args.getInt(0) , args.getInt(1) , new PointF((float) rmp1.getDouble("x"), (float) rmp1.getDouble("y")));
                    break;
                case COMMAND_LNGLAT_TO_SCREEN:
                     ReadableMap rmp2 = args.getMap(2);
                    lngLatToScreen(view, args.getInt(0) , args.getInt(1) , new LngLat((float) rmp2.getDouble("longitude"), (float) rmp2.getDouble("latitude")));
                    break;


                default:
                    throw new JSApplicationIllegalArgumentException("Invalid commandId " + commandId + " sent to " + getClass().getSimpleName());
            }
        } catch (Exception ex) {
            emitMapError(ex.getMessage(), "map_request_command_error");
        }
    }

    // Getters

    private void getGeoPosition(ReactTangramMapView view, int errorCallbackId , int successCallbackId) {
        try{
            WritableArray result = Arguments.createArray();
            LngLat pos = view.getMapController().getPosition();
            WritableMap wm = Arguments.createMap();
            wm.putDouble("latitude", pos.latitude);
            wm.putDouble("longitude", pos.longitude);
            result.pushMap(wm);
            fireSuccessCallback(successCallbackId, result);
        }
        catch(Exception ex){
            fireErrorCallback(errorCallbackId,-119, ex.getMessage(),"error_get_position");
        }

    }

    private void getTilt(ReactTangramMapView view, int errorCallbackId , int successCallbackId) {
        try{
            WritableArray result = Arguments.createArray();
            result.pushDouble(view.getMapController().getTilt());
            fireSuccessCallback(successCallbackId, result);
        }
        catch(Exception ex){
            fireErrorCallback(errorCallbackId,-115, ex.getMessage(),"error_get_tilt");
        }

    }

    private void getZoom(ReactTangramMapView view, int errorCallbackId , int successCallbackId) {
        try{
            WritableArray result = Arguments.createArray();
            result.pushDouble(view.getMapController().getZoom());
            fireSuccessCallback(successCallbackId, result);
        }
        catch(Exception ex){
            fireErrorCallback(errorCallbackId,-116, ex.getMessage(),"error_get_zoom");
        }

    }

    private void getRotation(ReactTangramMapView view, int errorCallbackId , int successCallbackId) {
        try{
            WritableArray result = Arguments.createArray();
            result.pushDouble(view.getMapController().getRotation());
            fireSuccessCallback(successCallbackId, result);
        }
        catch(Exception ex){
            fireErrorCallback(errorCallbackId,-117, ex.getMessage(),"error_get_rotation");
        }

    }

    private void getCameraType(ReactTangramMapView view, int errorCallbackId , int successCallbackId) {
        try{
            WritableArray result = Arguments.createArray();
            result.pushString(view.getMapController().getCameraType().name());
            fireSuccessCallback(successCallbackId, result);
        }
        catch(Exception ex){
            fireErrorCallback(errorCallbackId,-118, ex.getMessage(),"error_get_camera_type");
        }

    }


    // Setters
    private void setGeoPosition(ReactTangramMapView view, int errorCallbackId , int successCallbackId, LngLat point) {
        try {
            WritableArray result = Arguments.createArray();
            view.setGeoPosition(point);
            LngLat pos = view.getMapController().getPosition();
            WritableMap wm = Arguments.createMap();
            wm.putDouble("latitude", pos.latitude);
            wm.putDouble("longitude", pos.longitude);
            result.pushMap(wm);
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-120, ex.getMessage(),"error_set_position");
        }

    }

    private void setTilt(ReactTangramMapView view, int errorCallbackId , int successCallbackId, float tilt) {
        try {
            WritableArray result = Arguments.createArray();
            view.setTilt(tilt);
            result.pushDouble(view.getMapController().getTilt());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-121, ex.getMessage(),"error_set_tilt");
        }

    }

    private void setZoom(ReactTangramMapView view, int errorCallbackId , int successCallbackId, float zoom) {
        try {
            WritableArray result = Arguments.createArray();
            view.setZoom(zoom);
            result.pushDouble(view.getMapController().getZoom());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-121, ex.getMessage(),"error_set_zoom");
        }

    }

    private void setPickRadius(ReactTangramMapView view, int errorCallbackId , int successCallbackId, float radius) {
        try {
            WritableArray result = Arguments.createArray();
            view.setPickRadius(radius);
            result.pushDouble(radius);
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-122, ex.getMessage(),"error_set_pick_radius");
        }

    }

    private void setRotation(ReactTangramMapView view, int errorCallbackId , int successCallbackId, float rotation) {
        try {
            WritableArray result = Arguments.createArray();
            view.getMapController().setRotation(rotation);
            result.pushDouble(view.getMapController().getRotation());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-123, ex.getMessage(),"error_set_rotation");
        }

    }

    private void setCameraType(ReactTangramMapView view, int errorCallbackId , int successCallbackId, MapController.CameraType cameraType) {
        try {
            WritableArray result = Arguments.createArray();
            view.getMapController().setCameraType(cameraType);
            result.pushString(view.getMapController().getCameraType().name());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-124, ex.getMessage(),"error_set_camera_type");
        }

    }

    private void setPositionEase(ReactTangramMapView view, int errorCallbackId , int successCallbackId, LngLat point, int duration, MapController.EaseType easeType) {
        try {
            WritableArray result = Arguments.createArray();
            view.getMapController().setPositionEased(point, duration, easeType);
            LngLat pos = view.getMapController().getPosition();
            WritableMap wm = Arguments.createMap();
            wm.putDouble("latitude", pos.latitude);
            wm.putDouble("longitude", pos.longitude);
            result.pushMap(wm);
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-125, ex.getMessage(),"error_set_position_ease");
        }

    }

    private void setTiltEase(ReactTangramMapView view, int errorCallbackId , int successCallbackId, float tilt, int duration, MapController.EaseType easeType) {
        try {
            WritableArray result = Arguments.createArray();
            view.getMapController().setTiltEased(tilt, duration, easeType);
            result.pushDouble(view.getMapController().getTilt());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-126, ex.getMessage(),"error_set_tilt_ease");
        }

    }

    private void setZoomEase(ReactTangramMapView view, int errorCallbackId , int successCallbackId, float zoom, int duration, MapController.EaseType easeType) {
        try {
            WritableArray result = Arguments.createArray();
            view.getMapController().setZoomEased(zoom, duration, easeType);
            result.pushDouble(view.getMapController().getZoom());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-127, ex.getMessage(),"error_set_zoom_ease");
        }

    }

    private void setRotationEase(ReactTangramMapView view, int errorCallbackId , int successCallbackId, float rotation, int duration, MapController.EaseType easeType) {
        try {
            WritableArray result = Arguments.createArray();
            view.getMapController().setRotationEased(rotation, duration, easeType);
            result.pushDouble(view.getMapController().getRotation());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-128, ex.getMessage(),"error_set_rotation_ease");
        }

    }

    private void pickFeature(ReactTangramMapView view, int errorCallbackId , int successCallbackId, float posx, float posy) {
        try {
            WritableArray result = Arguments.createArray();
            view.getMapController().pickFeature(posx, posy);
            //result.pushDouble(view.getMapController().getRotation());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-129, ex.getMessage(),"error_pick_feature");
        }

    }

    private void pickLabel(ReactTangramMapView view, int errorCallbackId , int successCallbackId, float posx, float posy) {
        try {
            WritableArray result = Arguments.createArray();
            view.getMapController().pickLabel(posx, posy);
            //result.pushDouble(view.getMapController().getRotation());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-130, ex.getMessage(),"error_pick_label");
        }

    }

    private void pickMarker(ReactTangramMapView view, int errorCallbackId , int successCallbackId, float posx, float posy) {
        try {
            WritableArray result = Arguments.createArray();
            view.getMapController().pickMarker(posx, posy);
            //result.pushDouble(view.getMapController().getRotation());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-131, ex.getMessage(),"error_pick_marker");
        }

    }

    private void addDataLayer(ReactTangramMapView view, int errorCallbackId , int successCallbackId, String name, @Nullable String jsonData) {
        try {
            WritableArray result = Arguments.createArray();
            MapData mapData = view.getMapController().addDataLayer(name);
            if (jsonData != null && !jsonData.isEmpty()) {
                mapData.addGeoJson(jsonData);
            }

            result.pushString(mapData.name());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-132, ex.getMessage(),"error_add_datalayer");
        }

    }

    private void addPointToMapDataLayer(ReactTangramMapView view, int errorCallbackId , int successCallbackId, String name, LngLat point, @Nullable Map<String, String> propeties) {
        try {
            WritableArray result = Arguments.createArray();
            MapData mapData = view.getMapController().addDataLayer(name);
            mapData.addPoint(point, propeties);
            result.pushString(mapData.name());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-133, ex.getMessage(),"error_add_point_to_datalayer");
        }

    }

    private void addPolygonMapDataLayer(ReactTangramMapView view, int errorCallbackId , int successCallbackId, String name, List<List<LngLat>> points, @Nullable Map<String, String> propeties) {
        try {
            WritableArray result = Arguments.createArray();
            MapData mapData = view.getMapController().addDataLayer(name);
            mapData.addPolygon(points, propeties);
            result.pushString(mapData.name());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-134, ex.getMessage(),"error_add_polygon_to_datalayer");
        }

    }

    private void addPolylineMapDataLayer(ReactTangramMapView view, int errorCallbackId , int successCallbackId, String name, List<LngLat> points, @Nullable Map<String, String> propeties) {
        try {
            WritableArray result = Arguments.createArray();
            MapData mapData = view.getMapController().addDataLayer(name);
            mapData.addPolyline(points, propeties);
            result.pushString(mapData.name());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-135, ex.getMessage(),"error_add_polyline_to_datalayer");
        }

    }

    private void clearMapDataLayer(ReactTangramMapView view, int errorCallbackId , int successCallbackId, String name) {
        try {
            WritableArray result = Arguments.createArray();
            MapData mapData = view.getMapController().addDataLayer(name);
            mapData.clear();
            result.pushString(mapData.name());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-136, ex.getMessage(),"error_clear_map_datalayer");
        }

    }

    private void removeMapDataLayer(ReactTangramMapView view, int errorCallbackId , int successCallbackId, String name) {
        try {
            WritableArray result = Arguments.createArray();
            MapData mapData = view.getMapController().addDataLayer(name);
            mapData.remove();
            result.pushString(mapData.name());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-137, ex.getMessage(),"error_remove_map_datalayer");
        }

    }

    private void addMapMarker(ReactTangramMapView view, int errorCallbackId , int successCallbackId) {
        try {
            WritableArray result = Arguments.createArray();

            Marker marker = view.addMarker();

            WritableMap wm = Arguments.createMap();
            wm.putDouble("markerID", marker.getMarkerId());
            wm.putBoolean("isVisible", marker.isVisible());
            result.pushMap(wm);


            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-138, ex.getMessage(),"error_add_map_marker");
        }

    }

    private void putMapMarker(ReactTangramMapView view, int errorCallbackId , int successCallbackId, boolean isVisible,
                              @Nullable Integer drawOrder,
                              @Nullable LngLat point, @Nullable Polygon polygon,
                              @Nullable Polyline polyline, @Nullable String style,
                              @Nullable Integer drawableID, @Nullable Drawable drawable) {
        try {
            WritableArray result = Arguments.createArray();

            Marker marker = view.addMarker();
            if (point != null) {
                marker.setPoint(point);
            }
            if (polygon != null) {
                marker.setPolygon(polygon);
            }
            if (polyline != null) {
                marker.setPolyline(polyline);
            }

            if (style != null && !style.isEmpty()) {
                if(style.startsWith("@@")){
                    marker.setStylingFromPath(style.replace("@@",""));
                }
                else {
                    marker.setStylingFromString(style);
                }
            }

            if (drawableID != null) {
                marker.setDrawable(drawableID);
            }

            if (drawable != null) {
                marker.setDrawable(drawable);
            }

            if (drawOrder != null) {
                marker.setDrawOrder(drawOrder);
            }

            marker.setVisible(isVisible);


            WritableMap wm = Arguments.createMap();
            wm.putDouble("markerID", marker.getMarkerId());
            wm.putBoolean("isVisible", marker.isVisible());
            result.pushMap(wm);


            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-139, ex.getMessage(),"error_put_map_marker");
        }

    }


    private void updateMapMarker(ReactTangramMapView view, int errorCallbackId , int successCallbackId, long markerID, boolean isVisible,
                                 @Nullable Integer drawOrder,
                                 @Nullable LngLat point, @Nullable Polygon polygon,
                                 @Nullable Polyline polyline, @Nullable String style,
                                 @Nullable Integer drawableID, @Nullable Drawable drawable) {
        try {
            WritableArray result = Arguments.createArray();
            Marker marker = view.getMarker(markerID);
            if (point != null) {
                marker.setPoint(point);
            }
            if (polygon != null) {
                marker.setPolygon(polygon);
            }
            if (polyline != null) {
                marker.setPolyline(polyline);
            }

            if (style != null && !style.isEmpty()) {
                if(style.startsWith("@@")){
                    marker.setStylingFromPath(style.replace("@@",""));
                }
                else {
                    marker.setStylingFromString(style);
                }

            }

            if (drawableID != null) {
                marker.setDrawable(drawableID);
            }

            if (drawable != null) {
                marker.setDrawable(drawable);
            }

            if (drawOrder != null) {
                marker.setDrawOrder(drawOrder);
            }

            marker.setVisible(isVisible);

            WritableMap wm = Arguments.createMap();
            wm.putDouble("markerID", marker.getMarkerId());
            wm.putBoolean("isVisible", marker.isVisible());
            result.pushMap(wm);

            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-140, ex.getMessage(),"error_update_map_marker");
        }

    }

    private void removeMarker(ReactTangramMapView view, int errorCallbackId , int successCallbackId, long markerID) {
        try {
            WritableArray result = Arguments.createArray();
            view.removeMarker(markerID);
            result.pushDouble(markerID);
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-141, ex.getMessage(),"error_remove_map_marker");
        }

    }

    private void removeAllMarkers(ReactTangramMapView view, int errorCallbackId , int successCallbackId) {
        try {
            WritableArray result = Arguments.createArray();
            view.removeAllMarkers();
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-142, ex.getMessage(),"error_remove_all_map_markers");
        }

    }

    private void setMarkerVisible(ReactTangramMapView view, int errorCallbackId , int successCallbackId, long markerID, boolean isVisible) {
        try {
            WritableArray result = Arguments.createArray();
            Marker marker = view.getMarker(markerID);

            boolean aresult =  marker.setVisible(isVisible);

            WritableMap wm = Arguments.createMap();
            wm.putDouble("markerID", marker.getMarkerId());
            wm.putBoolean("isVisible", marker.isVisible());
            wm.putBoolean("result", aresult);
            result.pushMap(wm);

            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-151, ex.getMessage(),"error_set_visible_map_marker");
        }

    }

    private void setMarkerDrawOrder(ReactTangramMapView view, int errorCallbackId , int successCallbackId, long markerID, int drawOrder) {
        try {
            WritableArray result = Arguments.createArray();
            Marker marker = view.getMarker(markerID);

            boolean aresult = marker.setDrawOrder(drawOrder);

            WritableMap wm = Arguments.createMap();
            wm.putDouble("markerID", marker.getMarkerId());
            wm.putBoolean("isVisible", marker.isVisible());
            wm.putBoolean("result", aresult);
            result.pushMap(wm);

            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-152, ex.getMessage(),"error_set_draw_order_map_marker");
        }

    }

    private void setMarkerPoint(ReactTangramMapView view, int errorCallbackId , int successCallbackId, long markerID, LngLat point) {
        try {
            WritableArray result = Arguments.createArray();
            Marker marker = view.getMarker(markerID);

            boolean aresult = marker.setPoint(point);

            WritableMap wm = Arguments.createMap();
            wm.putDouble("markerID", marker.getMarkerId());
            wm.putBoolean("isVisible", marker.isVisible());
            wm.putBoolean("result", aresult);
            result.pushMap(wm);

            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-153, ex.getMessage(),"error_set_point_map_marker");
        }

    }

    private void setMarkerPointEased(ReactTangramMapView view, int errorCallbackId , int successCallbackId, long markerID, LngLat point, int duration,@Nullable  MapController.EaseType easeType) {
        try {
            WritableArray result = Arguments.createArray();
            Marker marker = view.getMarker(markerID);

            boolean aresult  =  marker.setPointEased(point,duration,easeType);

            WritableMap wm = Arguments.createMap();
            wm.putDouble("markerID", marker.getMarkerId());
            wm.putBoolean("isVisible", marker.isVisible());
            wm.putBoolean("result", aresult);
            result.pushMap(wm);

            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-154, ex.getMessage(),"error_set_point_eased_map_marker");
        }

    }

    private void setMarkerPolygon(ReactTangramMapView view, int errorCallbackId , int successCallbackId, long markerID, Polygon polygon) {
        try {
            WritableArray result = Arguments.createArray();
            Marker marker = view.getMarker(markerID);

            boolean aresult = marker.setPolygon(polygon);

            WritableMap wm = Arguments.createMap();
            wm.putDouble("markerID", marker.getMarkerId());
            wm.putBoolean("isVisible", marker.isVisible());
            wm.putBoolean("result", aresult);
            result.pushMap(wm);

            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-155, ex.getMessage(),"error_set_polygon_map_marker");
        }

    }

    private void setMarkerPolyline(ReactTangramMapView view, int errorCallbackId , int successCallbackId, long markerID, Polyline polyline) {
        try {
            WritableArray result = Arguments.createArray();
            Marker marker = view.getMarker(markerID);

            boolean aresult = marker.setPolyline(polyline);

            WritableMap wm = Arguments.createMap();
            wm.putDouble("markerID", marker.getMarkerId());
            wm.putBoolean("isVisible", marker.isVisible());
            wm.putBoolean("result", aresult);
            result.pushMap(wm);

            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-156, ex.getMessage(),"error_set_polyline_map_marker");
        }

    }

    private void setMarkerStylingFromPath(ReactTangramMapView view, int errorCallbackId , int successCallbackId, long markerID, String stylePath) {
        try {
            WritableArray result = Arguments.createArray();
            Marker marker = view.getMarker(markerID);

            boolean aresult = marker.setStylingFromPath(stylePath);

            WritableMap wm = Arguments.createMap();
            wm.putDouble("markerID", marker.getMarkerId());
            wm.putBoolean("isVisible", marker.isVisible());
            wm.putBoolean("result", aresult);
            result.pushMap(wm);

            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-157, ex.getMessage(),"error_set_style_from_path_map_marker");
        }

    }

    private void setMarkerStylingFromString(ReactTangramMapView view, int errorCallbackId , int successCallbackId, long markerID, String style) {
        try {
            WritableArray result = Arguments.createArray();
            Marker marker = view.getMarker(markerID);

            boolean aresult = marker.setStylingFromString(style);

            WritableMap wm = Arguments.createMap();
            wm.putDouble("markerID", marker.getMarkerId());
            wm.putBoolean("isVisible", marker.isVisible());
            wm.putBoolean("result", aresult);
            result.pushMap(wm);

            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-158, ex.getMessage(),"error_set_style_from_string_map_marker");
        }

    }

    private void setMarkerBitmap(ReactTangramMapView view, int errorCallbackId , int successCallbackId, long markerID, String bitmapString) {
        try {
            WritableArray result = Arguments.createArray();
            Marker marker = view.getMarker(markerID);
            byte[] bytes =  Base64.decode(bitmapString, Base64.DEFAULT);
            Bitmap bitmap = BitmapFactory.decodeByteArray(bytes, 0, bytes.length);
            boolean aresult = marker.setDrawable(new BitmapDrawable(_context.getResources(),bitmap));

            WritableMap wm = Arguments.createMap();
            wm.putDouble("markerID", marker.getMarkerId());
            wm.putBoolean("isVisible", marker.isVisible());
            wm.putBoolean("result", aresult);
            result.pushMap(wm);

            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-159, ex.getMessage(),"error_set_bitmap_map_marker");
        }

    }



    private void requestRender(ReactTangramMapView view, int errorCallbackId , int successCallbackId) {
        try {
            WritableArray result = Arguments.createArray();
            view.getMapController().requestRender();
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-170, ex.getMessage(),"error_request_render");
        }

    }

    private void screenToLngLat(ReactTangramMapView view, int errorCallbackId , int successCallbackId, PointF spoint) {
        try {
            WritableArray result = Arguments.createArray();
            LngLat lngLat = view.getMapController().screenPositionToLngLat(spoint);
            result.pushMap(view.makeCoordinateEventData(lngLat));
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-171, ex.getMessage(),"error_map_screen_to_latlng");

        }

    }

    private void lngLatToScreen(ReactTangramMapView view, int errorCallbackId , int successCallbackId, LngLat lngLat) {
        try {
            WritableArray result = Arguments.createArray();
            PointF spoint = view.getMapController().lngLatToScreenPosition(lngLat);
            result.pushMap(view.makePositionEventData(spoint));
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-172, ex.getMessage(),"error_map_latlng_to_screen");

        }

    }


    private void updateSceneAsync(ReactTangramMapView view, int errorCallbackId , int successCallbackId, @Nullable ArrayList<SceneUpdate> sceneUpdates) {
        try {
            WritableArray result = Arguments.createArray();
            if(sceneUpdates != null){
                view.getSceneUpdates().clear();
                view.getSceneUpdates().addAll(sceneUpdates);
            }
            view.getMapController().updateSceneAsync(view.getSceneUpdates());
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-173, ex.getMessage(),"error_map_update_scene_async");
        }

    }

    private void loadSceneFile(ReactTangramMapView view, int errorCallbackId , int successCallbackId, String path , ArrayList<SceneUpdate> sceneUpdates) {
        try {
            WritableArray result = Arguments.createArray();
            view.reloadScene(path,false,sceneUpdates);

            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-174, ex.getMessage(),"error_map_load_scenefile");
        }

    }

    private void loadSceneFileAsync(ReactTangramMapView view, int errorCallbackId , int successCallbackId, String path, ArrayList<SceneUpdate> sceneUpdates) {
        try {
            WritableArray result = Arguments.createArray();
            view.reloadScene(path,true, sceneUpdates);
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-175, ex.getMessage(),"error_map_load_scenefile_async");
        }

    }

    private void isSimultaneousGestureAllowed(ReactTangramMapView view, int errorCallbackId , int successCallbackId, TouchInput.Gestures first, TouchInput.Gestures second) {
        try {
            WritableArray result = Arguments.createArray();
            boolean isallowed = view.getMapController().isSimultaneousGestureAllowed(first, second);
            result.pushBoolean(isallowed);
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-176, ex.getMessage(),"error_map_is_simultaneousgesture_allowed");

        }

    }

    private void useCachedGlState(ReactTangramMapView view, int errorCallbackId , int successCallbackId, boolean useCache) {
        try {
            WritableArray result = Arguments.createArray();
            view.getMapController().useCachedGlState(useCache);
            fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-177, ex.getMessage(),"error_map_use_cached_glstate");

        }

    }
    private Bitmap getResizedBitmap(Bitmap bm, int newWidth, int newHeight) {
        int width = bm.getWidth();
        int height = bm.getHeight();
        float scaleWidth = ((float) newWidth) / width;
        float scaleHeight = ((float) newHeight) / height;
        // CREATE A MATRIX FOR THE MANIPULATION
        Matrix matrix = new Matrix();
        // RESIZE THE BIT MAP
        matrix.postScale(scaleWidth, scaleHeight);

        // "RECREATE" THE NEW BITMAP
        Bitmap resizedBitmap = Bitmap.createBitmap(
                bm, 0, 0, width, height, matrix, false);
        bm.recycle();
        return resizedBitmap;
    }

    private void captureFrame(ReactTangramMapView view, final int errorCallbackId , final int successCallbackId,
                              boolean waiting, @Nullable final Integer compressRate,
                              @Nullable final Integer height, @Nullable final Integer width) {
        try {


            view.getMapController().captureFrame(new MapController.FrameCaptureCallback() {
                @Override
                public void onCaptured(Bitmap bitmap) {
                    final WritableArray result = Arguments.createArray();
                    ByteArrayOutputStream stream = new ByteArrayOutputStream();
                    Bitmap bmp =  bitmap;
                    if(height != null | width != null) {
                        bmp = getResizedBitmap(bitmap,
                                width != null ? width : bitmap.getWidth(),
                                height != null ? height : bitmap.getHeight());
                    }
                    bitmap.compress(Bitmap.CompressFormat.PNG, compressRate != null ? compressRate : 100, stream);
                    byte[] byteArray = stream.toByteArray();
                    final WritableMap result1 =  Arguments.createMap();
                    String encoded = Base64.encodeToString(byteArray, Base64.DEFAULT);
                    result1.putInt("height",bitmap.getHeight());
                    result1.putInt("width",bitmap.getHeight());
                    result1.putString("format","PNG");
                    result1.putString("base64Data",encoded);
                    result.pushMap(result1);
                    fireSuccessCallback(successCallbackId, result);
                }
            }, waiting);
            // fireSuccessCallback(successCallbackId, result);
        } catch (Exception ex) {
            fireErrorCallback(errorCallbackId,-178, ex.getMessage(),"error_map_capture_frame");

        }

    }


    void pushEvent(View view, String name, WritableMap data) {
        _context.getJSModule(RCTEventEmitter.class)
                .receiveEvent(view.getId(), name, data);
    }

    @Override
    public void updateExtraData(ReactTangramMapView view, Object extraData) {
        view.updateExtraData(extraData);
    }

    @Override
    public void addView(ReactTangramMapView parent, View child, int index) {
        parent.addFeature(child, index);
    }

    @Override
    public int getChildCount(ReactTangramMapView view) {
        return view.getFeatureCount();
    }

    @Override
    public View getChildAt(ReactTangramMapView view, int index) {
        return view.getFeatureAt(index);
    }

    @Override
    public void removeViewAt(ReactTangramMapView parent, int index) {
        parent.removeFeatureAt(index);
    }

    @Override
    public void onDropViewInstance(ReactTangramMapView view) {
        // view.doDestroy();
        super.onDropViewInstance(view);
    }




}

