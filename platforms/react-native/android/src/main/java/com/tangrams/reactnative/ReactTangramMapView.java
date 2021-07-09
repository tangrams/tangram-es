package com.tangrams.reactnative;

import android.content.Context;

import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.PointF;
import android.graphics.PorterDuff;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.support.annotation.Nullable;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import com.facebook.react.bridge.Arguments;
import com.facebook.react.bridge.LifecycleEventListener;

import com.facebook.react.bridge.WritableMap;
import com.facebook.react.bridge.WritableNativeMap;
import com.facebook.react.modules.core.DeviceEventManagerModule;
import com.facebook.react.uimanager.ThemedReactContext;
import com.facebook.react.uimanager.events.RCTEventEmitter;
import com.mapzen.tangram.HttpHandler;
import com.mapzen.tangram.LabelPickResult;
import com.mapzen.tangram.LngLat;
import com.mapzen.tangram.MapController;
import com.mapzen.tangram.MapController.SceneLoadListener;
import com.mapzen.tangram.MapView;
import com.mapzen.tangram.Marker;
import com.mapzen.tangram.MarkerPickResult;
import com.mapzen.tangram.SceneError;
import com.mapzen.tangram.SceneUpdate;
import com.mapzen.tangram.TouchInput;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
/**
 * Created by saeed tabrizi saeed@nowcando.com on 2/26/17.
 */
public class ReactTangramMapView extends FrameLayout
        implements LifecycleEventListener, SceneLoadListener, TouchInput.TapResponder,
        TouchInput.DoubleTapResponder, TouchInput.LongPressResponder, MapController.FeaturePickListener,
        TouchInput.ScaleResponder, TouchInput.PanResponder, TouchInput.ShoveResponder, TouchInput.RotateResponder,
        MapController.LabelPickListener, MapController.MarkerPickListener, MapController.ViewCompleteListener {
    private ArrayList<SceneUpdate> _sceneUpdates = new ArrayList<>();
    private ThemedReactContext context;
    private TangramMapViewManager _manager;
    private MapController _map;
    private MapView _mapView = null;
    private android.os.Handler _handler;
    private boolean _paused = false;
    private static final String TAG = "ReactTangramMapView";
    private boolean cacheEnabled = false;
    private GLSurfaceView glSurfaceView;
    private float pickRadius = 0.5f;
    private float minzoom = 1, maxzoom = 24, zoom = 14;
    private float mintilt = -360, maxtilt = 360, tilt = 0.5f;
    private float minrotate = -360, maxrotate = 360, rotate = 0;
    private long cacheSize = 0;
    private MapController.CameraType cameraType = MapController.CameraType.FLAT;
    private String cacheName = "tile_cache";
    private String _scenePath = "asset:///default_scene.yaml";
    private LngLat _geoposition = new LngLat(51.0097, 35.7053); // init as Tehran
    private LngLat defaultposition = new LngLat(51.0097, 35.7053); // init as Tehran
    private Map<Long, Marker> markers = new HashMap<Long, Marker>();

    private ProgressBar mapLoadingProgressBar;
    private RelativeLayout mapLoadingLayout;
    private ImageView cacheImageView;
    private Boolean isMapLoaded = false;
    private Integer loadingBackgroundColor = null;
    private Integer loadingIndicatorColor = null;
    private final int baseMapPadding = 50;

    public ReactTangramMapView(ThemedReactContext reactContext, Context appContext, TangramMapViewManager manager) {
        super(appContext);
        context = reactContext;
        this._manager = manager;
        _handler = new android.os.Handler();

    }

    // Lifecycle methods

    //@Override
    public void onAfterUpdateTransaction() {
        if (_mapView != null) {
            return;
        }
        setupMapView();
        _paused = false;
        if (_mapView != null) {
            _mapView.onResume();
        }

        if (glSurfaceView != null) {
            // glSurfaceView.onResume();
            // glSurfaceView.setZOrderMediaOverlay(true);
        }
        _manager.getContext().addLifecycleEventListener(this);
    }

    public void onDrop() {
        if (_mapView == null) {
            return;
        }
        _manager.getContext().removeLifecycleEventListener(this);
        if (!_paused) {
            _paused = true;
            _mapView.onPause();
            //  glSurfaceView.onPause();
        }
        destroyMapView();
        _mapView = null;
    }

    @Override
    public void onHostResume() {
        _paused = false;

        if (_mapView != null) {
            _mapView.onResume();
        }

        if (glSurfaceView != null) {
            //  glSurfaceView.setZOrderMediaOverlay(true);
            // glSurfaceView.onResume();
        }

    }

    @Override
    public void onHostPause() {
        _paused = true;
        if (_mapView != null) {
            _mapView.onPause();
        }
        if (glSurfaceView != null) {
            // glSurfaceView.onPause();
        }

    }

    @Override
    public void onHostDestroy() {
        onDrop();
    }

    // Initialization

    private void setupMapView() {
        // if(_mapView != null) { return ;} // already registered view , we return

        if (_mapView == null) {
            _mapView = new MapView(this.getContext()); // initialize the mapview

            //_mapView.setBackgroundColor(Color.YELLOW);
            LayoutParams mapViewLayoutParams = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
            this.addView(_mapView, 0, mapViewLayoutParams);

            // glSurfaceView = ((GLSurfaceView) _mapView.gls);
            // glSurfaceView.setZOrderMediaOverlay(true);
            _mapView.onCreate(null);
            this.onHostResume();
            this.reloadScene(this._scenePath,true,this._sceneUpdates);
            _mapView.invalidate();
        }
    }

    /*
    * setup event handlers
    * */
    void setupEventHandlers() {
        if (_map != null) {

            _map.setTapResponder(this);
            _map.setLongPressResponder(this);
            _map.setDoubleTapResponder(this);
            _map.setViewCompleteListener(this);
            _map.setPanResponder(this);
            _map.setRotateResponder(this);
            _map.setScaleResponder(this);
            _map.setShoveResponder(this);
            _map.setLabelPickListener(this);
            _map.setMarkerPickListener(this);
            _map.setFeaturePickListener(this);

        }

    }

    private void destroyMapView() {
        //_mapView.(this);
        if (_map != null) {
            _map.setTapResponder(null);
            _map.setDoubleTapResponder(null);
            _map.setLabelPickListener(null);
            _map.setScaleResponder(null);
            _map.setShoveResponder(null);
            _map.setPanResponder(null);
            _map.setRotateResponder(null);
            _map.setMarkerPickListener(null);
            _map.setViewCompleteListener(null);
            _map.setFeaturePickListener(null);
            _map.setLongPressResponder(null);
            _map = null;
        }
        _mapView.onDestroy();
    }

    private void emitMapError(String message, String type) {
        WritableMap error = Arguments.createMap();
        error.putString("message", message);
        error.putString("type", type);

        context.getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter.class).emit("onError", error);
    }

    private void pushEvent(View view, String name, WritableMap data) {
        context.getJSModule(RCTEventEmitter.class).receiveEvent(view.getId(), name, data);
    }

    private ProgressBar getMapLoadingProgressBar() {
        if (this.mapLoadingProgressBar == null) {
            this.mapLoadingProgressBar = new ProgressBar(getContext());
            this.mapLoadingProgressBar.setIndeterminate(true);
        }
        if (this.loadingIndicatorColor != null) {
            this.setLoadingIndicatorColor(this.loadingIndicatorColor);
        }
        return this.mapLoadingProgressBar;
    }

    private RelativeLayout getMapLoadingLayoutView() {
        if (this.mapLoadingLayout == null) {
            this.mapLoadingLayout = new RelativeLayout(getContext());
            this.mapLoadingLayout.setBackgroundColor(Color.LTGRAY);
            this.addView(this.mapLoadingLayout, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT));

            RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                    RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
            params.addRule(RelativeLayout.CENTER_IN_PARENT);
            this.mapLoadingLayout.addView(this.getMapLoadingProgressBar(), params);

            this.mapLoadingLayout.setVisibility(View.INVISIBLE);
        }
        this.setLoadingBackgroundColor(this.loadingBackgroundColor);
        return this.mapLoadingLayout;
    }

    private ImageView getCacheImageView() {
        if (this.cacheImageView == null) {
            this.cacheImageView = new ImageView(getContext());
            this.addView(this.cacheImageView, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT));
            this.cacheImageView.setVisibility(View.INVISIBLE);
        }
        return this.cacheImageView;
    }

    private void removeCacheImageView() {
        if (this.cacheImageView != null) {
            ((ViewGroup) this.cacheImageView.getParent()).removeView(this.cacheImageView);
            this.cacheImageView = null;
        }
    }

    private void removeMapLoadingProgressBar() {
        if (this.mapLoadingProgressBar != null) {
            ((ViewGroup) this.mapLoadingProgressBar.getParent()).removeView(this.mapLoadingProgressBar);
            this.mapLoadingProgressBar = null;
        }
    }

    private void removeMapLoadingLayoutView() {
        this.removeMapLoadingProgressBar();
        if (this.mapLoadingLayout != null) {
            ((ViewGroup) this.mapLoadingLayout.getParent()).removeView(this.mapLoadingLayout);
            this.mapLoadingLayout = null;
        }
    }

    private void cacheView() {
        if (this.cacheEnabled) {
            final ImageView cacheImageView = this.getCacheImageView();
            final RelativeLayout mapLoadingLayout = this.getMapLoadingLayoutView();
            cacheImageView.setVisibility(View.INVISIBLE);
            mapLoadingLayout.setVisibility(View.VISIBLE);
            if (this.isMapLoaded) {

                /*this.map.snapshot(new GoogleMap.SnapshotReadyCallback() {
                    @Override public void onSnapshotReady(Bitmap bitmap) {
                        cacheImageView.setImageBitmap(bitmap);
                        cacheImageView.setVisibility(View.VISIBLE);
                        mapLoadingLayout.setVisibility(View.INVISIBLE);
                    }
                });*/
            }
        } else {
            this.removeCacheImageView();
            if (this.isMapLoaded) {
                this.removeMapLoadingLayoutView();
            }
        }
    }

    public void reloadScene(String value, boolean isAsync,ArrayList<SceneUpdate> sceneUpdates) {
        try {
            if (_mapView == null) {
                return;
            }
            Log.d(TAG, "new scenePath:" + value);
            boolean needInit = _map == null;
            //_manager.pushEvent(this, "onSceneReloadingScene", new WritableNativeMap());
            _map = _mapView.getMap(this);
            //_manager.pushEvent(this, "onSceneReloadScene", new WritableNativeMap());
            _scenePath = value;
            if(isAsync){
                _map.loadSceneFileAsync(_scenePath,sceneUpdates);
            }
            else{
                _map.loadSceneFile(_scenePath,sceneUpdates);
            }

            if(needInit){
                _map.setZoom(this.normalizedZoom(this.zoom));
                _map.setPosition(this.normalizedPosition(this._geoposition));
                _map.setHttpHandler(getHttpHandler(cacheSize, cacheName));
                // setup events
                setupEventHandlers();
            }


        } catch (Exception ex) {
            emitMapError(ex.getMessage(), isAsync ?"map_load_scene_async_error" : "map_load_scene_error");
            Log.d(TAG, "scenePath:" + value);
        }

    }

    public ArrayList<SceneUpdate> getSceneUpdates(){
        return this._sceneUpdates;
    }

    protected HttpHandler getHttpHandler(long cacheSize, String cacheName) {
        try {
            File cacheDir = this.context.getExternalCacheDir();
            if (cacheDir != null && cacheDir.exists() && cacheSize > 0) {
                return new HttpHandler(new File(cacheDir, cacheName), cacheSize * 1024 * 1024);
            }
            return new HttpHandler();
        } catch (Exception ex) {
            emitMapError(ex.getMessage(), "map_http_creation_error");
        }
        return null;
    }

    protected MapController getMapController() {
        return this._map;
    }

    public void enableMapLoadingView(boolean loadingEnabled) {
        if (loadingEnabled && !this.isMapLoaded) {
            this.getMapLoadingLayoutView().setVisibility(View.VISIBLE);
        }
    }

    public void enableUserTracking(boolean userTrackingEnabled) {

    }

    public WritableMap makeMarkerEventData(Marker marker) {
        WritableMap markerEvent = new WritableNativeMap();
        markerEvent.putDouble("markerId", marker.getMarkerId());
        markerEvent.putBoolean("isVisible", marker.isVisible());
        return markerEvent;
    }

    public WritableMap makeCoordinateEventData(LngLat point) {
        WritableMap coordinate = new WritableNativeMap();
        coordinate.putDouble("latitude", point.latitude);
        coordinate.putDouble("longitude", point.longitude);
        return coordinate;
    }

    public WritableMap makePositionEventData(PointF point) {
        WritableMap position = new WritableNativeMap();
        position.putDouble("x", point.x);
        position.putDouble("y", point.y);
        return position;
    }

    public WritableMap makePositionCoordinateEventData(LngLat point, PointF screenPoint) {
        WritableMap positioncoordinate = new WritableNativeMap();
        positioncoordinate.putMap("coordinate", makeCoordinateEventData(point));
        positioncoordinate.putMap("position", makePositionEventData(screenPoint));
        return positioncoordinate;
    }

    public WritableMap makeClickEventData(LngLat point) {

        PointF screenPoint = _map.lngLatToScreenPosition(point);
        WritableMap event = makePositionCoordinateEventData(point, screenPoint);

        return event;
    }

    public void setLoadingBackgroundColor(Integer loadingBackgroundColor) {
        this.loadingBackgroundColor = loadingBackgroundColor;

        if (this.mapLoadingLayout != null) {
            if (loadingBackgroundColor == null) {
                this.mapLoadingLayout.setBackgroundColor(Color.WHITE);
            } else {
                this.mapLoadingLayout.setBackgroundColor(this.loadingBackgroundColor);
            }
        }
    }

    public void setLoadingIndicatorColor(Integer loadingIndicatorColor) {
        this.loadingIndicatorColor = loadingIndicatorColor;
        if (this.mapLoadingProgressBar != null) {
            Integer color = loadingIndicatorColor;
            if (color == null) {
                color = Color.parseColor("#606060");
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                ColorStateList progressTintList = ColorStateList.valueOf(loadingIndicatorColor);
                ColorStateList secondaryProgressTintList = ColorStateList.valueOf(loadingIndicatorColor);
                ColorStateList indeterminateTintList = ColorStateList.valueOf(loadingIndicatorColor);

                this.mapLoadingProgressBar.setProgressTintList(progressTintList);
                this.mapLoadingProgressBar.setSecondaryProgressTintList(secondaryProgressTintList);
                this.mapLoadingProgressBar.setIndeterminateTintList(indeterminateTintList);
            } else {
                PorterDuff.Mode mode = PorterDuff.Mode.SRC_IN;
                if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.GINGERBREAD_MR1) {
                    mode = PorterDuff.Mode.MULTIPLY;
                }
                if (this.mapLoadingProgressBar.getIndeterminateDrawable() != null)
                    this.mapLoadingProgressBar.getIndeterminateDrawable().setColorFilter(color, mode);
                if (this.mapLoadingProgressBar.getProgressDrawable() != null)
                    this.mapLoadingProgressBar.getProgressDrawable().setColorFilter(color, mode);
            }
        }
    }

    float ensureRange(float value, float min, float max) {
        return Math.min(Math.max(value, min), max);
    }

    public float normalizedRotate(float value) {
        return ensureRange(value, minrotate, maxrotate);
    }

    public float normalizedTilt(float value) {
        return ensureRange(value, mintilt, maxtilt);
    }

    public float normalizedZoom(float value) {
        return ensureRange(value, minzoom, maxzoom);
        //return value;
    }

    public LngLat normalizedPosition(LngLat value) {
        //longitude -180,180
        //latitude -90,90
        return new LngLat(ensureRange((float) value.longitude, -180, 180),
                ensureRange((float) value.latitude, -90, 90));
        //return value;

    }

    public LngLat getGeoPosition() {
        return _map.getPosition();
    }

    public float getTilt() {
        return _map.getTilt();
    }

    public float getRotation() {
        return _map.getRotation();
    }

    public float getZoom() {
        return _map.getZoom();
    }

    public MapController.CameraType getCameraType() {
        return _map.getCameraType();
    }

    public boolean hasMap() {
        return this._map != null;
    }

    public void setCacheSize(Long value) {
        this.cacheSize = value;
        if (_map == null)
            return;
        _map.setHttpHandler(getHttpHandler(cacheSize, cacheName));
    }

    public void setCacheName(String value) {
        this.cacheName = value;
        if (_map == null)
            return;
        _map.setHttpHandler(getHttpHandler(cacheSize, cacheName));
    }

    public void setScenePath(String scenePath) {
        this._scenePath = scenePath;
        if (_map == null) {
            return;
        }
        this.reloadScene(this._scenePath,true,this._sceneUpdates);
    }

    public void setSceneUpdates(ArrayList<SceneUpdate> sceneUpdates) {
        this._sceneUpdates = sceneUpdates;
        if (_map == null) {
            return;
        }
        this._map.updateSceneAsync(this._sceneUpdates);
    }

    public void setGeoPosition(LngLat lngLat) {
        this._geoposition = lngLat;
        if (_map == null) {
            return;
        }
        _map.setPosition(normalizedPosition(this._geoposition));
    }

    public void setMinRotate(Float value) {
        this.minrotate = value;
    }

    public void setMaxRotate(Float value) {
        this.maxrotate = value;
    }

    public void setMaxTilt(Float value) {
        this.maxtilt = value;
    }

    public void setMinTilt(Float value) {
        this.mintilt = value;
    }

    public void setMaxZoom(Float value) {
        this.maxzoom = value;
    }

    public void setMinZoom(Float value) {
        this.minzoom = value;
    }

    /**
     * Set the zoom level of the map view
     * @param value Zoom level; lower values show more area
     */
    public void setZoom(Float value) {
        this.zoom = value;
        if (_map == null) {
            return;
        }
        _map.setZoom(normalizedZoom((float) value));
    }

    /**
     * Set the tilt angle of the view
     * @param value Tilt angle in radians; 0 corresponds to straight down
     */
    public void setTilt(Float value) {
        this.tilt = value;
        if (_map == null) {
            return;
        }
        _map.setTilt(normalizedTilt((float) value));
    }

    /**
     * Set the rotation of the view
     * @param value Counter-clockwise rotation in radians; 0 corresponds to North pointing up
     */
    public void setRotate(Float value) {
        this.tilt = value;
        if (_map == null) {
            return;
        }
        _map.setRotation(normalizedRotate((float) value));
    }

    public void setPickRadius(Float value) {
        this.pickRadius = value;
        if (_map == null) {
            return;
        }
        _map.setPickRadius((float) value);
    }

    public void setCameraType(MapController.CameraType cameraType) {
        this.cameraType = cameraType;
        if (_map == null) {
            return;
        }
        _map.setCameraType(this.cameraType);
    }

    public void setRegionBounds(LngLat topLeft, LngLat topRight, LngLat bottomLeft, LngLat bottomRight) {

    }

    public Marker addMarker() {
        Marker marker = this._map.addMarker();
        markers.put(marker.getMarkerId(), marker);
        return marker;
    }

    public Marker getMarker(long id) {
        Marker marker = this.markers.get(id);
        return marker;
    }

    public void removeMarker(long id) {
        _map.removeMarker(this.getMarker(id));
        markers.remove(id);
    }

    public void removeAllMarkers() {
        _map.removeAllMarkers();
        markers.clear();
    }

    private boolean _handleDoubleTap = true, _handleSingleTapUp = true, _handleSingleTapConfirmed = true,
            _handlePan = true, _handleFling = true, _handleScale = true, _handleRotate = true, _handleShove = true;

    public void setHandleDoubleTap(boolean enable) {
        this._handleDoubleTap = enable;
    }

    public void setHandleSingleTapUp(boolean enable) {
        this._handleSingleTapUp = enable;
    }

    public void setHandleSingleTapConfirmed(boolean enable) {
        this._handleSingleTapConfirmed = enable;
    }

    public void setHandlePan(boolean enable) {
        this._handlePan = enable;
    }

    public void setHandleFling(boolean enable) {
        this._handleFling = enable;
    }

    public void setHandleScale(boolean enable) {
        this._handleScale = enable;
    }

    public void setHandleRotate(boolean enable) {
        this._handleRotate = enable;
    }

    public void setHandleShove(boolean enable) {
        this._handleShove = enable;
    }


    @Override
    public void onFeaturePick(Map<String, String> dmap, float v, float v1) {
        PointF screenPosition = new PointF(v, v1);
        LngLat point = _map.screenPositionToLngLat(screenPosition);

        WritableMap event = makePositionCoordinateEventData(point, screenPosition);
        WritableMap featurePick = new WritableNativeMap();
        WritableMap properties = new WritableNativeMap();

        for (Map.Entry<String, String> item : dmap.entrySet()) {
            properties.putString(item.getKey(), item.getValue());
        }

        featurePick.putMap("properties", properties);

        event.putMap("featurePick", featurePick);
        event.putString("action", "feature-pick");

        _manager.pushEvent(this, "onFeaturePick", event);
    }

    @Override
    public void onLabelPick(LabelPickResult labelPickResult, float v, float v1) {

        PointF screenPosition = new PointF(v, v1);
        LngLat point = _map.screenPositionToLngLat(screenPosition);

        WritableMap event = makePositionCoordinateEventData(point, screenPosition);
        WritableMap labelPick = new WritableNativeMap();
        WritableMap properties = new WritableNativeMap();

        labelPick.putMap("coordinate", makeCoordinateEventData(labelPickResult.getCoordinates()));

        labelPick.putString("type", labelPickResult.getType().name());

        for (Map.Entry<String, String> item : labelPickResult.getProperties().entrySet()) {
            properties.putString(item.getKey(), item.getValue());
        }

        labelPick.putMap("properties", properties);

        event.putMap("labelPick", labelPick);
        event.putString("action", "label-pick");

        _manager.pushEvent(this, "onLabelPick", event);
    }

    @Override
    public void onMarkerPick(MarkerPickResult markerPickResult, float v, float v1) {
        PointF screenPosition = new PointF(v, v1);
        LngLat point = _map.screenPositionToLngLat(screenPosition);

        WritableMap event = makePositionCoordinateEventData(point, screenPosition);
        WritableMap markerPick = new WritableNativeMap();
        WritableMap marker = new WritableNativeMap();

        markerPick.putMap("coordinate", makeCoordinateEventData(markerPickResult.getCoordinates()));
        markerPick.putMap("marker", makeMarkerEventData(markerPickResult.getMarker()));

        event.putMap("markerPick", markerPick);
        event.putString("action", "marker-pick");

        _manager.pushEvent(this, "onMarkerPick", event);
    }

    @Override
    public boolean onDoubleTap(float v, float v1) {
        LngLat point = _map.screenPositionToLngLat(new PointF(v, v1));
        WritableMap event = makeClickEventData(point);
        event.putString("action", "double-tap");
        _manager.pushEvent(this, "onDoubleTap", event);
        return !_handleDoubleTap;
    }

    @Override
    public void onLongPress(float v, float v1) {
        LngLat point = _map.screenPositionToLngLat(new PointF(v, v1));
        WritableMap event = makeClickEventData(point);
        event.putString("action", "long-press");
        _manager.pushEvent(this, "onLongPress", event);
    }

    @Override
    public boolean onSingleTapUp(float v, float v1) {
        LngLat point = _map.screenPositionToLngLat(new PointF(v, v1));
        WritableMap event = makeClickEventData(point);
        event.putString("action", "single-tapup");
        _manager.pushEvent(this, "onSingleTapUp", event);
        return !_handleSingleTapUp;
    }

    @Override
    public boolean onSingleTapConfirmed(float v, float v1) {
        LngLat point = _map.screenPositionToLngLat(new PointF(v, v1));
        WritableMap event = makeClickEventData(point);
        event.putString("action", "single-tap-confirmed");
        _manager.pushEvent(this, "onSingleTapConfirmed", event);
        return !_handleSingleTapConfirmed;
    }

    @Override
    public void onViewComplete() {
        _manager.pushEvent(this, "onViewComplete", new WritableNativeMap());
        /*  PointF spoint =  new PointF(v,v1);
        PointF epoint =  new PointF(v2,v3);
        LngLat start = _map.screenPositionToLngLat(spoint);
        LngLat end = _map.screenPositionToLngLat(epoint);
        this.onRegionChangeComplete(start,end,spoint,epoint);*/

    }

    @Override
    public boolean onPan(float v, float v1, float v2, float v3) {
        WritableMap event = new WritableNativeMap();
        event.putDouble("startX", v);
        event.putDouble("startY", v1);
        event.putDouble("endX", v2);
        event.putDouble("endY", v3);
        _manager.pushEvent(this, "onPan", event);

        return !_handlePan;
    }

    @Override
    public boolean onFling(float v, float v1, float v2, float v3) {
        WritableMap event = new WritableNativeMap();
        event.putDouble("posX", v);
        event.putDouble("posY", v1);
        event.putDouble("velocityX", v2);
        event.putDouble("velocityY", v3);
        _manager.pushEvent(this, "onFling", event);
        return !_handleFling;

    }

    @Override
    public boolean onScale(float v, float v1, float v2, float v3) {
        WritableMap event = new WritableNativeMap();
        event.putDouble("x", v);
        event.putDouble("y", v1);
        event.putDouble("scale", v2);
        event.putDouble("velocity", v3);
        _manager.pushEvent(this, "onScale", event);
        return !_handleScale;
    }

    @Override
    public boolean onRotate(float v, float v1, float v2) {
        WritableMap event = new WritableNativeMap();
        event.putDouble("x", v);
        event.putDouble("y", v1);
        event.putDouble("rotation", v2);
        _manager.pushEvent(this, "onRotate", event);
        return !_handleRotate;
    }

    @Override
    public boolean onShove(float v) {
        WritableMap event = new WritableNativeMap();
        event.putDouble("distance", v);
        _manager.pushEvent(this, "onShove", event);
        return !_handleShove;
    }

    public void addFeature(View view, int index) {
        this.addView(view, index);
    }

    public int getFeatureCount() {
        return this.getChildCount();
    }

    public View getFeatureAt(int index) {
        return this.getChildAt(index);
    }

    public void removeFeatureAt(int index) {
        this.removeViewAt(index);
    }

    public void updateExtraData(Object extraData) {
    }

    @Override
    public void onSceneReady(int i, SceneError sceneError) {
        if (_mapView == null || _map == null) {
            return;
        }

        try {
            _mapView.getRootView().invalidate();
            WritableNativeMap scenewp =   new WritableNativeMap();
            scenewp.putInt("id",i);
            if(sceneError!=null){
                scenewp.putString("error",sceneError.getError().toString());
            }
            _manager.pushEvent(this, "onSceneReady", scenewp );
            markers.clear();
            _map.removeAllMarkers();

        } catch (Exception ex) {
            emitMapError("Tangram Scene ready error", "map_scene_ready_error");
        }

    }


}
