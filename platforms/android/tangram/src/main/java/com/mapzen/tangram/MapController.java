package com.mapzen.tangram;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.PointF;
import android.graphics.Rect;
import android.os.Build;
import android.os.Handler;
import android.util.ArrayMap;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.LongSparseArray;
import android.view.MotionEvent;
import android.view.View;

import com.mapzen.tangram.TouchInput.Gestures;
import com.mapzen.tangram.viewholder.GLViewHolder;
import com.mapzen.tangram.networking.DefaultHttpHandler;
import com.mapzen.tangram.networking.HttpHandler;

import java.io.IOException;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import androidx.annotation.IntRange;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * {@code MapController} is the main class for interacting with a Tangram map.
 */
public class MapController {


    public boolean handleGesture(View mapView, MotionEvent ev) {
        return touchInput.onTouch(mapView, ev);
    }

    /**
     * Options for interpolating map parameters
     */
    @Keep
    public enum EaseType {
        LINEAR,
        CUBIC,
        QUINT,
        SINE,
    }

    /**
     * Options for changing the appearance of 3D geometry
     */
    public enum CameraType {
        PERSPECTIVE,
        ISOMETRIC,
        FLAT,
    }

    /**
     * Options representing an error generated after from the map controller
     */
    public enum Error {
        NONE,
        SCENE_UPDATE_PATH_NOT_FOUND,
        SCENE_UPDATE_PATH_YAML_SYNTAX_ERROR,
        SCENE_UPDATE_VALUE_YAML_SYNTAX_ERROR,
        NO_VALID_SCENE,
    }

    protected static EaseType DEFAULT_EASE_TYPE = EaseType.CUBIC;

    /**
     * Options for enabling debug rendering features
     */
    public enum DebugFlag {
        FREEZE_TILES,
        PROXY_COLORS,
        TILE_BOUNDS,
        TILE_INFOS,
        LABELS,
        TANGRAM_INFOS,
        DRAW_ALL_LABELS,
        TANGRAM_STATS,
        SELECTION_BUFFER,
    }

    enum MapRegionChangeState {
        IDLE,
        JUMPING,
        ANIMATING,
    }

    /**
     * Interface for listening to scene load status information.
     * Triggered after a call of {@link #loadSceneYamlAsync(String, String, List<SceneUpdate>)} or
     * {@link #loadSceneFileAsync(String, List<SceneUpdate>)} or {@link #loadSceneFile(String, List<SceneUpdate>)}
     * Listener should be set with {@link #setSceneLoadListener(SceneLoadListener)}
     * The callbacks will be run on the main (UI) thread.
     */
    @Keep
    public interface SceneLoadListener {
        /**
         * Received when a scene load or update finishes. If sceneError is not null then the operation did not succeed.
         * @param sceneId The identifier returned by {@link #loadSceneYamlAsync(String, String, List<SceneUpdate>)} or
         * {@link #loadSceneFileAsync(String, List<SceneUpdate>)}.
         * @param sceneError A {@link SceneError} holding error information, or null if no error occurred.
         */
        void onSceneReady(final int sceneId, final SceneError sceneError);
    }

    public interface CameraAnimationCallback {
        void onFinish();
        void onCancel();
    }

    /**
     * Callback for {@link #captureFrame(FrameCaptureCallback, boolean) }
     */
    public interface FrameCaptureCallback {
        /**
         * Called on the ui-thread when a frame was captured.
         */
        void onCaptured(@NonNull final Bitmap bitmap);
    }

    /**
     * Capture MapView as Bitmap.
     * @param waitForCompleteView Delay the capture until the view is fully loaded and
     *                            no ease- or label-animation is running.
     */
    public void captureFrame(@NonNull final FrameCaptureCallback callback, final boolean waitForCompleteView) {
        mapRenderer.captureFrame(callback, waitForCompleteView);
        requestRender();
    }

    /**
     * Construct a MapController
     * @param context The application Context in which the map will function; the asset
     * bundle for this activity must contain all the local files that the map
     * will need.
     */
    protected MapController(@NonNull Context context) {
        if (Build.VERSION.SDK_INT > 18) {
            clientTileSources = new ArrayMap<>();
        } else {
            clientTileSources = new HashMap<>();
        }
        markers = new LongSparseArray<>();

        // Get configuration info from application
        DisplayMetrics displayMetrics = context.getResources().getDisplayMetrics();
        AssetManager assetManager = context.getAssets();

        // Parse font file description
        //FontConfig.init();

        nativeMap = new NativeMap(this, assetManager);
        nativeMap.setPixelScale(displayMetrics.density);
    }

    /**
     * Responsible to configure {@link MapController} configuration on the ui thread.
     * Must be called from the ui thread post instantiation of {@link MapController}
     * @param viewHolder GLViewHolder for the map display
     * @param handler {@link HttpHandler} to initialize httpHandler for network handling
     */
    void UIThreadInit(@NonNull final GLViewHolder viewHolder, @Nullable final HttpHandler handler) {

        // Use the DefaultHttpHandler if none is provided
        if (handler == null) {
            httpHandler = new DefaultHttpHandler();
        } else {
            httpHandler = handler;
        }

        Context context = viewHolder.getView().getContext();

        uiThreadHandler = new Handler(context.getMainLooper());
        mapRenderer = new MapRenderer(this, uiThreadHandler);

        // Set up MapView
        this.viewHolder = viewHolder;
        viewHolder.setRenderer(mapRenderer);
        viewHolder.setRenderMode(GLViewHolder.RenderMode.RENDER_WHEN_DIRTY);

        touchInput = new TouchInput(context);

        touchInput.setPanResponder(getPanResponder());
        touchInput.setScaleResponder(getScaleResponder());
        touchInput.setRotateResponder(getRotateResponder());
        touchInput.setShoveResponder(getShoveResponder());

        touchInput.setSimultaneousDetectionDisabled(Gestures.SHOVE, Gestures.ROTATE);
        touchInput.setSimultaneousDetectionDisabled(Gestures.ROTATE, Gestures.SHOVE);
        touchInput.setSimultaneousDetectionDisabled(Gestures.SHOVE, Gestures.SCALE);
        touchInput.setSimultaneousDetectionDisabled(Gestures.SHOVE, Gestures.PAN);
        touchInput.setSimultaneousDetectionDisabled(Gestures.SCALE, Gestures.LONG_PRESS);
    }

    /**
     * Responsible to dispose internals of MapController during Map teardown.
     * If client code extends MapController and overrides this method, then it must call super.dispose()
     */
    protected synchronized void dispose() {

        Log.e("TANGRAM", ">>> dispose");
        nativeMap.shutdown();
        Log.e("TANGRAM", "<<< http requests: " + httpRequestHandles.size());

        for (MapData mapData : clientTileSources.values()) {
            mapData.remove();
        }

        Log.e("TANGRAM", "<<< 1");
        clientTileSources.clear();
        Log.e("TANGRAM", "<<< 2");
        markers.clear();
        Log.e("TANGRAM", "<<< 3");

        // Dispose all listener and callbacks associated with mapController
        // This will help prevent leaks of references from the client code, possibly used in these
        // listener/callbacks
        touchInput = null;
        mapChangeListener = null;
        featurePickListener = null;
        sceneLoadListener = null;
        labelPickListener = null;
        markerPickListener = null;
        cameraAnimationCallback = null;

        // Prevent any calls to native functions - except dispose.
        final NativeMap disposingNativeMap = nativeMap;
        nativeMap = null;

        // NOTE: It is possible for the MapView held by a ViewGroup to be removed, calling detachFromWindow which
        // stops the Render Thread associated with GLSurfaceView, possibly resulting in leaks from render thread
        // queue. Because of the above, destruction lifecycle of the GLSurfaceView will not be triggered.
        // To avoid the above senario, nativeDispose is called from the UIThread.
        //
        // Since all gl resources will be freed when GLSurfaceView is deleted this is safe until
        // we support sharing gl contexts.
        disposingNativeMap.dispose();
        Log.e("TANGRAM", "<<< disposed");

    }

    /**
     * Returns the {@link GLViewHolder} instance for the client application, which can be further used to get the underlying
     * GLSurfaceView or Client provided implementation for GLViewHolder, to forward any explicit view controls, example Transformations, etc.
     * @return GLViewHolder used by the Map Renderer
     */
    @Nullable
    public GLViewHolder getGLViewHolder() {
        return viewHolder;
    }

    /**
     * Load a new scene file synchronously.
     * Use {@link #setSceneLoadListener(SceneLoadListener)} for notification when the new scene is
     * ready.
     * @param path Location of the YAML scene file within the application assets
     * @return Scene ID An identifier for the scene being loaded, the same value will be passed to
     * {@link SceneLoadListener#onSceneReady(int sceneId, SceneError sceneError)} when loading is complete.
     */
    public int loadSceneFile(final String path) {
        return loadSceneFile(path, null);
    }

    /**
     * Load a new scene file asynchronously.
     * Use {@link #setSceneLoadListener(SceneLoadListener)} for notification when the new scene is
     * ready.
     * @param path Location of the YAML scene file within the application assets
     * @return Scene ID An identifier for the scene being loaded, the same value will be passed to
     * {@link SceneLoadListener#onSceneReady(int sceneId, SceneError sceneError)} when loading is complete.
     */
    public int loadSceneFileAsync(final String path) {
        return loadSceneFileAsync(path, null);
    }

    /**
     * Load a new scene file synchronously.
     * If scene updates triggers an error, they won't be applied.
     * Use {@link #setSceneLoadListener(SceneLoadListener)} for notification when the new scene is
     * ready.
     * @param path Location of the YAML scene file within the application assets
     * @param sceneUpdates List of {@code SceneUpdate}
     * @return Scene ID An identifier for the scene being loaded, the same value will be passed to
     * {@link SceneLoadListener#onSceneReady(int sceneId, SceneError sceneError)} when loading is complete.
     */
    public int loadSceneFile(final String path, @Nullable final List<SceneUpdate> sceneUpdates) {
        final String[] updateStrings = bundleSceneUpdates(sceneUpdates);
        final int sceneId = nativeMap.loadScene(path, updateStrings);
        removeAllMarkers();
        requestRender();
        return sceneId;
    }

    /**
     * Load a new scene file asynchronously.
     * If scene updates triggers an error, they won't be applied.
     * Use {@link #setSceneLoadListener(SceneLoadListener)} for notification when the new scene is
     * ready.
     * @param path Location of the YAML scene file within the application assets
     * @param sceneUpdates List of {@code SceneUpdate}
     * @return Scene ID An identifier for the scene being loaded, the same value will be passed to
     * {@link SceneLoadListener#onSceneReady(int sceneId, SceneError sceneError)} when loading is complete.
     */
    public int loadSceneFileAsync(final String path, @Nullable final List<SceneUpdate> sceneUpdates) {
        final String[] updateStrings = bundleSceneUpdates(sceneUpdates);
        final int sceneId = nativeMap.loadSceneAsync(path, updateStrings);
        removeAllMarkers();
        requestRender();
        return sceneId;
    }

    /**
     * Load a new scene synchronously, provided an explicit yaml scene string to load
     * If scene updates triggers an error, they won't be applied.
     * Use {@link #setSceneLoadListener(SceneLoadListener)} for notification when the new scene is
     * ready.
     * @param yaml YAML scene String
     * @param resourceRoot base path to resolve relative URLs
     * @param sceneUpdates List of {@code SceneUpdate}
     * @return Scene ID An identifier for the scene being loaded, the same value will be passed to
     */
    public int loadSceneYaml(final String yaml, final String resourceRoot,
                             @Nullable final List<SceneUpdate> sceneUpdates) {
        final String[] updateStrings = bundleSceneUpdates(sceneUpdates);
        final int sceneId = nativeMap.loadSceneYaml(yaml, resourceRoot, updateStrings);
        removeAllMarkers();
        requestRender();
        return sceneId;
    }

    /**
     * Load a new scene asynchronously, provided an explicit yaml scene string to load
     * If scene updates triggers an error, they won't be applied.
     * Use {@link #setSceneLoadListener(SceneLoadListener)} for notification when the new scene is
     * ready.
     * @param yaml YAML scene String
     * @param resourceRoot base path to resolve relative URLs
     * @param sceneUpdates List of {@code SceneUpdate}
     * @return Scene ID An identifier for the scene being loaded, the same value will be passed to
     */
    public int loadSceneYamlAsync(final String yaml, final String resourceRoot,
                                  @Nullable final List<SceneUpdate> sceneUpdates) {
        final String[] updateStrings = bundleSceneUpdates(sceneUpdates);
        final int sceneId = nativeMap.loadSceneYamlAsync(yaml, resourceRoot, updateStrings);
        removeAllMarkers();
        requestRender();
        return sceneId;
    }

    /**
     * Set the camera position of the map view
     * @param update CameraUpdate to modify current camera position
     */
    public void updateCameraPosition(@NonNull final CameraUpdate update) {
        updateCameraPosition(update, 0, DEFAULT_EASE_TYPE,null);
    }

    /**
     * Animate the camera position of the map view with the default easing function
     * @param update CameraUpdate to update current camera position
     * @param duration Time in milliseconds to ease to the updated position
     */
    public void updateCameraPosition(@NonNull final CameraUpdate update, final int duration) {
        updateCameraPosition(update, duration, DEFAULT_EASE_TYPE, null);
    }

    /**
     * Animate the camera position of the map view with an easing function
     * @param update CameraUpdate to update current camera position
     * @param duration Time in milliseconds to ease to the updated position
     * @param ease Type of easing to use
     */
    public void updateCameraPosition(@NonNull final CameraUpdate update, final int duration, @NonNull final EaseType ease) {
        updateCameraPosition(update, duration, ease, null);
    }

    /**
     * Animate the camera position of the map view and run a callback when the animation completes
     * @param update CameraUpdate to update current camera position
     * @param duration Time in milliseconds to ease to the updated position
     * @param cb Callback that will run when the animation is finished or canceled
     */
    public void updateCameraPosition(@NonNull final CameraUpdate update, final int duration, @NonNull final CameraAnimationCallback cb) {
        updateCameraPosition(update, duration, DEFAULT_EASE_TYPE, cb);
    }

    /**
     * Animate the camera position of the map view with an easing function and run a callback when
     * the animation completes
     * @param update CameraUpdate to update current camera position
     * @param duration Time in milliseconds to ease to the updated position
     * @param ease Type of easing to use
     * @param cb Callback that will run when the animation is finished or canceled
     */
    public void updateCameraPosition(@NonNull final CameraUpdate update, final int duration, @NonNull final EaseType ease, @Nullable final CameraAnimationCallback cb) {
        if (duration > 0) {
            setMapRegionState(MapRegionChangeState.ANIMATING);
        } else {
            setMapRegionState(MapRegionChangeState.JUMPING);
        }
        setPendingCameraAnimationCallback(cb);
        final float seconds = duration / 1000.f;
        nativeMap.updateCameraPosition(update.set, update.longitude, update.latitude, update.zoom,
                update.zoomBy, update.rotation, update.rotationBy, update.tilt, update.tiltBy,
                update.boundsLon1, update.boundsLat1, update.boundsLon2, update.boundsLat2, update.padding,
                seconds, ease.ordinal());
    }

    /**
     * Smoothly animate over an arc to a new camera position for the map view
     * FlyTo duration is calculated assuming speed of 1 unit and distance between the current and new positions
     * @param position CameraPosition of the destination
     * @param callback Callback that will run when the animation is finished or canceled
     */
    public void flyToCameraPosition(@NonNull CameraPosition position, @Nullable final CameraAnimationCallback callback) {
        flyToCameraPosition(position, 0, callback);
    }

    /**
     * Smoothly animate over an arc to a new camera position for the map view in provided time duration
     * @param position CameraPosition of the destination
     * @param duration Time in milliseconds of the animation
     * @param callback Callback that will run when the animation is finished or canceled
     */
    public void flyToCameraPosition(@NonNull final CameraPosition position, final int duration, @Nullable final CameraAnimationCallback callback) {
        flyToCameraPosition(position, duration, callback, 1);
    }

    /**
     * Smoothly animate over an arc to a new camera position for the map view
     * FlyTo duration is calculated using speed and distance between the current and new positions
     * @param position CameraPosition of the destination
     * @param callback Callback that will run when the animation is finished or canceled
     * @param speed Scaling factor for animation duration (recommended range is 0.1 - 10)
     */
    public void flyToCameraPosition(@NonNull final CameraPosition position, @Nullable final CameraAnimationCallback callback, final float speed) {
        flyToCameraPosition(position, -1, callback, speed);
    }

    private void flyToCameraPosition(@NonNull final CameraPosition position, final int duration, @Nullable final CameraAnimationCallback callback, final float speed) {
        if (duration == 0) {
            setMapRegionState(MapRegionChangeState.JUMPING);
        } else {
            setMapRegionState(MapRegionChangeState.ANIMATING);
        }
        setPendingCameraAnimationCallback(callback);
        final float seconds = duration / 1000.f;
        nativeMap.flyTo(position.longitude, position.latitude, position.zoom, seconds, speed);
    }

    private void setPendingCameraAnimationCallback(final CameraAnimationCallback callback) {
        synchronized (cameraAnimationCallbackLock) {
            // Wrap the callback to run corresponding map change events.
            pendingCameraAnimationCallback = new CameraAnimationCallback() {
                @Override
                public void onFinish() {
                    setMapRegionState(MapRegionChangeState.IDLE);
                    if (callback != null) {
                        callback.onFinish();
                    }
                }

                @Override
                public void onCancel() {
                    // Possible camera update was cancelled in between, so should account for this map change
                    setMapRegionState(MapRegionChangeState.IDLE);
                    if (callback != null) {
                        callback.onCancel();
                    }
                }
            };
        }
    }

    /**
     * Get the {@link CameraPosition} of the map view
     * @return The current camera position
     */
    @NonNull
    public CameraPosition getCameraPosition() {
        return getCameraPosition(new CameraPosition());
    }

    /**
     * Get the {@link CameraPosition} of the map view
     * @param out CameraPosition to be reused as the output
     * @return the current camera position of the map view
     */
    @NonNull
    public CameraPosition getCameraPosition(@NonNull final CameraPosition out) {
        nativeMap.getCameraPosition(out);
        return out;
    }

    /**
     * Cancel current camera animation
     */
    public void cancelCameraAnimation() {
        nativeMap.cancelCameraAnimation();
    }

    /**
     * Get the {@link CameraPosition} that that encloses the given bounds with at least the given amount of padding on each side.
     * @param sw south-west coordinate
     * @param ne northwest coordinate
     * @param padding The minimum distance to keep between the bounds and the edges of the view
     * @return The enclosing camera position
     */
    @NonNull
    public CameraPosition getEnclosingCameraPosition(@NonNull LngLat sw, @NonNull LngLat ne, @NonNull Rect padding) {
        return getEnclosingCameraPosition(sw, ne, padding, new CameraPosition());
    }

    /**
     * Get the {@link CameraPosition} that that encloses the given bounds with at least the given amount of padding on each side.
     * @param sw south-west coordinate
     * @param ne northwest coordinate
     * @param padding The minimum distance to keep between the bounds and the edges of the view
     * @param out CameraPosition to be reused as the output
     * @return The enclosing camera position
     */
    @NonNull
    public CameraPosition getEnclosingCameraPosition(@NonNull LngLat sw, @NonNull LngLat ne, @NonNull Rect padding, @NonNull final CameraPosition out) {
        nativeMap.getEnclosingCameraPosition(sw, ne, padding, out);
        return out;
    }

    /**
     * Set the camera type for the map view
     * @param type A {@code CameraType}
     */
    public void setCameraType(@NonNull final CameraType type) {
        nativeMap.setCameraType(type.ordinal());
    }

    /**
     * Get the camera type currently in use for the map view
     * @return A {@code CameraType}
     */
    public CameraType getCameraType() {
        int nativeCameraType = nativeMap.getCameraType();
        return CameraType.values()[nativeCameraType];
    }

    /**
     * Get the minimum zoom level of the map view. The default minimum zoom is 0.
     * @return The zoom level
     */
    public float getMinimumZoomLevel() {
        return nativeMap.getMinZoom();
    }

    /**
     * Set the minimum zoom level of the map view.
     *
     * Values less than the default minimum zoom will be clamped. Assigning a value greater than the
     * current maximum zoom will set the maximum zoom to this value.
     * @param minimumZoom The zoom level
     */
    public void setMinimumZoomLevel(float minimumZoom) {
        nativeMap.setMinZoom(minimumZoom);
    }

    /**
     * Get the maximum zoom level of the map view. The default maximum zoom is 20.5.
     * @return The zoom level
     */
    public float getMaximumZoomLevel() {
        return nativeMap.getMaxZoom();
    }

    /**
     * Set the maximum zoom level of the map view.
     *
     * Values greater than the default maximum zoom will be clamped. Assigning a value less than the
     * current minimum zoom will set the minimum zoom to this value.
     * @param maximumZoom The zoom level
     */
    public void setMaximumZoomLevel(float maximumZoom) {
        nativeMap.setMaxZoom(maximumZoom);
    }

    /**
     * Find the geographic coordinates corresponding to the given position on screen
     * @param screenPosition Position in pixels from the top-left corner of the map area
     * @return LngLat corresponding to the given point, or null if the screen position
     * does not intersect a geographic location (this can happen at high tilt angles).
     */
    @Nullable
    public LngLat screenPositionToLngLat(@NonNull final PointF screenPosition) {
        LngLat result = new LngLat();
        if (screenPositionToLngLat(screenPosition, result)) {
            return result;
        }
        return null;
    }

    /**
     * Find the geographic coordinates corresponding to the given position on screen.
     * @param screenPosition Position in pixels from the top-left corner of the map area.
     * @param lngLatOut LngLat object to hold the result.
     * @return True if a LngLat result was found, false if the screen position does not intersect
     * a geographic location (this can happen at high tilt angles).
     */
    public boolean screenPositionToLngLat(@NonNull final PointF screenPosition, @NonNull final LngLat lngLatOut) {
        return nativeMap.screenPositionToLngLat(screenPosition.x, screenPosition.y, lngLatOut);
    }

    /**
     * Find the position on screen corresponding to the given geographic coordinates
     * @param lngLat Geographic coordinates
     * @return Position in pixels from the top-left corner of the map area (the point
     * may not lie within the viewable screen area)
     */
    @NonNull
    public PointF lngLatToScreenPosition(@NonNull final LngLat lngLat) {
        PointF screenPosition = new PointF();
        lngLatToScreenPosition(lngLat, screenPosition, false);
        return screenPosition;
    }

    /**
     * Find the position on screen corresponding to the given geographic coordinates
     * @param lngLat Geographic coordinates.
     * @param screenPositionOut Point object to hold result.
     * @param clipToViewport If true, results that are outside of the viewport will be clipped to a
     *                       point on the edge of the viewport in the direction towards the location.
     * @return True if the resulting point is inside the viewport, otherwise false.
     */
    public boolean lngLatToScreenPosition(@NonNull final LngLat lngLat, @NonNull final PointF screenPositionOut, boolean clipToViewport) {
        return nativeMap.lngLatToScreenPosition(lngLat.longitude, lngLat.latitude, screenPositionOut, clipToViewport);
    }

    /**
     * Construct a collection of drawable map features.
     * @param name The name of the data collection. Once added to a map, features from this
     * {@code MapData} will be available from a data source with this name, just like a data source
     * specified in a scene file. You cannot create more than one data source with the same name.
     * If you call {@code addDataLayer} with the same name more than once, the same {@code MapData}
     * object will be returned.
     */
    public MapData addDataLayer(final String name) {
        return addDataLayer(name, false);
    }

    /**
     * Construct a collection of drawable map features.
     * @param name The name of the data collection. Once added to a map, features from this
     * @param generateCentroid boolean to control <a href=
     * "https://mapzen.com/documentation/tangram/sources/#generate_label_centroids"> label centroid
     * generation</a> for polygon geometry
     * {@code MapData} will be available from a data source with this name, just like a data source
     * specified in a scene file. You cannot create more than one data source with the same name.
     * If you call {@code addDataLayer} with the same name more than once, the same {@code MapData}
     * object will be returned.
     */
    @NonNull
    public MapData addDataLayer(final String name, final boolean generateCentroid) {
        MapData mapData = clientTileSources.get(name);
        if (mapData != null) {
            return mapData;
        }
        final long pointer = nativeMap.addClientDataSource(name, generateCentroid);
        if (pointer == 0) {
            throw new RuntimeException("Unable to create new data source");
        }
        mapData = new MapData(name, pointer, this);
        clientTileSources.put(name, mapData);
        return mapData;
    }

    /**
     * For package-internal use only; remove a {@code MapData} from this map
     * @param mapData The {@code MapData} to remove
     */
    void removeDataLayer(@NonNull final MapData mapData) {
        clientTileSources.remove(mapData.name);
        if (mapData.pointer == 0) {
            throw new RuntimeException("Tried to remove a MapData that was already disposed");
        }
        nativeMap.removeClientDataSource(mapData.pointer);
    }

    /**
     * Manually trigger a re-draw of the map view
     *
     * Typically this does not need to be called from outside Tangram, see {@link #setRenderMode(int)}.
     */
    @Keep
    public void requestRender() {
        viewHolder.requestRender();
    }

    /**
     * Set whether the map view re-draws continuously
     *
     * Typically this does not need to be called from outside Tangram. The map automatically re-renders when the view
     * changes or when any animation in the map requires rendering.
     * @param renderMode Either 1, to render continuously, or 0, to render only when needed.
     */
    @Keep
    public void setRenderMode(@IntRange(from=0,to=1) final int renderMode) {
        switch (renderMode) {
            case 0:
                viewHolder.setRenderMode(GLViewHolder.RenderMode.RENDER_WHEN_DIRTY);
                break;
            case 1:
                viewHolder.setRenderMode(GLViewHolder.RenderMode.RENDER_CONTINUOUSLY);
                break;
            default:
        }
    }

    /**
     * Get the {@link TouchInput} for this map.
     *
     * {@code TouchInput} allows you to configure how gestures can move the map. You can set custom
     * responders for any gesture type in {@code TouchInput} to override or extend the default
     * behavior.
     *
     * Note that {@code MapController} assigns the default gesture responders for the pan, rotate,
     * scale, and shove gestures. If you set custom responders for these gestures, the default
     * responders will be replaced and those gestures will no longer move the map. To customize
     * these gesture responders and preserve the default map movement behavior: create a new gesture
     * responder, get the responder for that gesture from the {@code MapController}, and then in the
     * new responder call the corresponding methods on the {@code MapController} gesture responder.
     * @return The {@code TouchInput}.
     */
    public TouchInput getTouchInput() {
        return touchInput;
    }

    /**
     * Get a responder for pan gestures
     */
    public TouchInput.PanResponder getPanResponder() {
        return new TouchInput.PanResponder() {
            @Override
            public boolean onPanBegin() {
                setMapRegionState(MapRegionChangeState.JUMPING);
                return true;
            }

            @Override
            public boolean onPan(final float startX, final float startY, final float endX, final float endY) {
                setMapRegionState(MapRegionChangeState.JUMPING);
                nativeMap.handlePanGesture(startX, startY, endX, endY);
                return true;
            }

            @Override
            public boolean onPanEnd() {
                setMapRegionState(MapRegionChangeState.IDLE);
                return true;
            }

            @Override
            public boolean onFling(final float posX, final float posY, final float velocityX, final float velocityY) {
                nativeMap.handleFlingGesture(posX, posY, velocityX, velocityY);
                return true;
            }

            @Override
            public boolean onCancelFling() {
                cancelCameraAnimation();
                return true;
            }
        };
    }

    /**
     * Get a responder for rotate gestures
     */
    public TouchInput.RotateResponder getRotateResponder() {
        return new TouchInput.RotateResponder() {
            @Override
            public boolean onRotateBegin() {
                setMapRegionState(MapRegionChangeState.JUMPING);
                return true;
            }

            @Override
            public boolean onRotate(final float x, final float y, final float rotation) {
                setMapRegionState(MapRegionChangeState.JUMPING);
                nativeMap.handleRotateGesture(x, y, rotation);
                return true;
            }

            @Override
            public boolean onRotateEnd() {
                setMapRegionState(MapRegionChangeState.IDLE);
                return true;
            }
        };
    }

    /**
     * Get a responder for scale gestures
     */
    public TouchInput.ScaleResponder getScaleResponder() {
        return new TouchInput.ScaleResponder() {
            @Override
            public boolean onScaleBegin() {
                setMapRegionState(MapRegionChangeState.JUMPING);
                return true;
            }

            @Override
            public boolean onScale(final float x, final float y, final float scale, final float velocity) {
                setMapRegionState(MapRegionChangeState.JUMPING);
                nativeMap.handlePinchGesture(x, y, scale, velocity);
                return true;
            }

            @Override
            public boolean onScaleEnd() {
                setMapRegionState(MapRegionChangeState.IDLE);
                return true;
            }
        };
    }

    /**
     * Get a responder for shove (vertical two-finger drag) gestures
     */
    public TouchInput.ShoveResponder getShoveResponder() {
        return new TouchInput.ShoveResponder() {
            @Override
            public boolean onShoveBegin() {
                setMapRegionState(MapRegionChangeState.JUMPING);
                return true;
            }

            @Override
            public boolean onShove(final float distance) {
                setMapRegionState(MapRegionChangeState.JUMPING);
                nativeMap.handleShoveGesture(distance);
                return true;
            }

            @Override
            public boolean onShoveEnd() {
                setMapRegionState(MapRegionChangeState.IDLE);
                return true;
            }
        };
    }

    /**
     * Set the radius to use when picking features on the map. The default radius is 0.5 dp.
     * @param radius The radius in dp (density-independent pixels).
     */
    public void setPickRadius(final float radius) {
        nativeMap.setPickRadius(radius);
    }

    /**
     * Set a listener for feature pick events
     * @param listener The {@link FeaturePickListener} to call
     */
    public void setFeaturePickListener(@Nullable final FeaturePickListener listener) {
        featurePickListener = listener;
    }

    /**
     * Set a listener for scene update error statuses
     * @param listener The {@link SceneLoadListener} to call after scene has loaded
     */
    public void setSceneLoadListener(@Nullable final SceneLoadListener listener) {
        sceneLoadListener = listener;
    }

    /**
     * Set a listener for label pick events
     * @param listener The {@link LabelPickListener} to call
     */
    public void setLabelPickListener(@Nullable final LabelPickListener listener) {
        labelPickListener = (listener == null) ? null : new LabelPickListener() {
            @Override
            public void onLabelPickComplete(final LabelPickResult labelPickResult) {
                uiThreadHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        listener.onLabelPickComplete(labelPickResult);
                    }
                });
            }
        };
    }

    /**
     * Set a listener for marker pick events
     * @param listener The {@link MarkerPickListener} to call
     */
    public void setMarkerPickListener(@Nullable final MarkerPickListener listener) {
        markerPickListener = (listener == null) ? null : new MarkerPickListener() {
            @Override
            public void onMarkerPickComplete(final MarkerPickResult markerPickResult) {
                uiThreadHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        listener.onMarkerPickComplete(markerPickResult);
                    }
                });
            }
        };
    }

    /**
     * Query the map for geometry features at the given screen coordinates; results will be returned
     * in a callback to the object set by {@link #setFeaturePickListener(FeaturePickListener)}
     * @param posX The horizontal screen coordinate
     * @param posY The vertical screen coordinate
     */
    public void pickFeature(final float posX, final float posY) {
        if (featurePickListener != null) {
            nativeMap.pickFeature(posX, posY);
        }
    }

    /**
     * Query the map for labeled features at the given screen coordinates; results will be returned
     * in a callback to the object set by {@link #setLabelPickListener(LabelPickListener)}
     * @param posX The horizontal screen coordinate
     * @param posY The vertical screen coordinate
     */
    public void pickLabel(final float posX, final float posY) {
        if (labelPickListener != null) {
            nativeMap.pickLabel(posX, posY);
        }
    }

    /**
     * Query the map for a {@link Marker} at the given screen coordinates; results will be returned
     * in a callback to the object set by {@link #setMarkerPickListener(MarkerPickListener)}
     * @param posX The horizontal screen coordinate
     * @param posY The vertical screen coordinate
     */
    public void pickMarker(final float posX, final float posY) {
        if (markerPickListener != null) {
            nativeMap.pickMarker(posX, posY);
        }
    }

    /**
     * Adds a {@link Marker} to the map which can be used to dynamically add points and polylines
     * to the map.
     * @return Newly created {@link Marker} object.
     */
    @NonNull
    public Marker addMarker() {
        final long markerId = nativeMap.markerAdd();

        final Marker marker = new Marker(viewHolder.getView().getContext(), markerId, this);
        markers.put(markerId, marker);

        return marker;
    }

    /**
     * Removes the passed in {@link Marker} from the map.
     * Alias of Marker{@link #removeMarker(long)}
     * @param marker to remove from the map.
     * @return whether or not the marker was removed
     */
    public boolean removeMarker(@NonNull final Marker marker) {
        return this.removeMarker(marker.getMarkerId());
    }

    /**
     * Removes the passed in {@link Marker} from the map.
     * @param markerId to remove from the map.
     * @return whether or not the marker was removed
     */
    public boolean removeMarker(final long markerId) {
        checkId(markerId);
        markers.remove(markerId);
        return nativeMap.markerRemove(markerId);
    }

    /**
     * Remove all the {@link Marker} objects from the map.
     */
    public void removeAllMarkers() {
        nativeMap.markerRemoveAll();

        // Invalidate all markers so their ids are unusable
        for (int i = 0; i < markers.size(); i++) {
            final Marker marker = markers.valueAt(i);
            marker.invalidate();
        }

        markers.clear();
    }

    /**
     * Set a listener for map change events
     * @param listener The {@link MapChangeListener} to call when the map change events occur
     *                due to camera updates or user interaction
     */
    public void setMapChangeListener(@Nullable final MapChangeListener listener) {
        mapChangeListener = listener;
    }


    void setMapRegionState(MapRegionChangeState state) {

        if (mapChangeListener != null) {
            switch (currentState) {
                case IDLE:
                    if (state == MapRegionChangeState.JUMPING) {
                        mapChangeListener.onRegionWillChange(false);
                    } else if (state == MapRegionChangeState.ANIMATING){
                        mapChangeListener.onRegionWillChange(true);
                    }
                    break;
                case JUMPING:
                    if (state == MapRegionChangeState.IDLE) {
                        mapChangeListener.onRegionDidChange(false);
                    } else if (state == MapRegionChangeState.JUMPING) {
                        mapChangeListener.onRegionIsChanging();
                    }
                    break;
                case ANIMATING:
                    if (state == MapRegionChangeState.IDLE) {
                        mapChangeListener.onRegionDidChange(true);
                    } else if (state == MapRegionChangeState.ANIMATING) {
                        mapChangeListener.onRegionIsChanging();
                    }
                    break;
            }
        }
        currentState = state;
    }

    /**
     * Enqueue a Runnable to be executed synchronously on the rendering thread
     * @param r Runnable to run
     */
    public void queueEvent(@NonNull final Runnable r) {
        viewHolder.queueEvent(r);
    }

    /**
     * Make a debugging feature active or inactive
     * @param flag The feature to set
     * @param on True to activate the feature, false to deactivate
     */
    public void setDebugFlag(@NonNull final DebugFlag flag, final boolean on) {
        nativeMap.setDebugFlag(flag.ordinal(), on);
    }

    /**
     * Set whether the OpenGL state will be cached between subsequent frames. This improves
     * rendering efficiency, but can cause errors if your application code makes OpenGL calls.
     * @param use Whether to use a cached OpenGL state; false by default
     */
    public void useCachedGlState(final boolean use) {
        nativeMap.useCachedGlState(use);
    }

    /**
     * Sets an opaque background color used as default color when a scene is being loaded
     * @param red red component of the background color
     * @param green green component of the background color
     * @param blue blue component of the background color
     */
    public void setDefaultBackgroundColor(final float red, final float green, final float blue) {
        nativeMap.setDefaultBackgroundColor(red, green, blue);
    }

    // Package private methods
    // =======================

    void onLowMemory() {
        nativeMap.onLowMemory();
    }

    void checkId(final long id) {
        if (id <= 0) {
            throw new RuntimeException("Tried to perform an operation on an invalid id!"
                    + " This means you may have used an object that has been disposed and is no"
                    + " longer valid.");
        }
    }

    @Nullable
    private String[] bundleSceneUpdates(@Nullable final List<SceneUpdate> sceneUpdates) {
        if (sceneUpdates == null) {
            return null;
        }

        final String[] updateStrings = new String[sceneUpdates.size() * 2];
        int index = 0;
        for (final SceneUpdate sceneUpdate : sceneUpdates) {
            updateStrings[index++] = sceneUpdate.getPath();
            updateStrings[index++] = sceneUpdate.getValue();
        }
        return updateStrings;
    }

    boolean setMarkerStylingFromString(final long markerId, final String styleString) {
        checkId(markerId);
        return nativeMap.markerSetStylingFromString(markerId, styleString);
    }

    boolean setMarkerStylingFromPath(final long markerId, final String path) {
        checkId(markerId);
        return nativeMap.markerSetStylingFromPath(markerId, path);
    }

    boolean setMarkerBitmap(final long markerId, Bitmap bitmap, float density) {
        checkId(markerId);
        return nativeMap.markerSetBitmap(markerId, bitmap, density);
    }

    boolean setMarkerPoint(final long markerId, final double lng, final double lat) {
        checkId(markerId);
        return nativeMap.markerSetPoint(markerId, lng, lat);
    }

    boolean setMarkerPointEased(final long markerId, final double lng, final double lat, final int duration,
                                @NonNull final EaseType ease) {
        checkId(markerId);
        final float seconds = duration / 1000.f;
        return nativeMap.markerSetPointEased(markerId, lng, lat, seconds, ease.ordinal());
    }

    boolean setMarkerPolyline(final long markerId, final double[] coordinates, final int count) {
        checkId(markerId);
        return nativeMap.markerSetPolyline(markerId, coordinates, count);
    }

    boolean setMarkerPolygon(final long markerId, final double[] coordinates, final int[] rings, final int count) {
        checkId(markerId);
        return nativeMap.markerSetPolygon(markerId, coordinates, rings, count);
    }

    boolean setMarkerVisible(final long markerId, final boolean visible) {
        checkId(markerId);
        return nativeMap.markerSetVisible(markerId, visible);
    }

    boolean setMarkerDrawOrder(final long markerId, final int drawOrder) {
        checkId(markerId);
        return nativeMap.markerSetDrawOrder(markerId, drawOrder);
    }


    // Networking methods
    // ==================

    @Keep
    void cancelUrlRequest(final long requestHandle) {
        Object request = httpRequestHandles.remove(requestHandle);
        if (request != null) {
            httpHandler.cancelRequest(request);
        }
    }

    @Keep
    void startUrlRequest(@NonNull final String url, final long requestHandle) {

        final HttpHandler.Callback callback = new HttpHandler.Callback() {
            @Override
            public void onFailure(@Nullable final IOException e) {
                if (httpRequestHandles.remove(requestHandle) == null) { return; }
                String msg = (e == null) ? "" : e.getMessage();
                nativeMap.onUrlComplete(requestHandle, null, msg);
            }

            @Override
            public void onResponse(final int code, @Nullable final byte[] rawDataBytes) {
                if (httpRequestHandles.remove(requestHandle) == null) { return; }
                if (code >= 200 && code < 300) {
                    nativeMap.onUrlComplete(requestHandle, rawDataBytes, null);
                } else {
                    nativeMap.onUrlComplete(requestHandle, null,
                            "Unexpected response code: " + code + " for URL: " + url);
                }
            }

            @Override
            public void onCancel() {
                if (httpRequestHandles.remove(requestHandle) == null) { return; }
                nativeMap.onUrlComplete(requestHandle, null, null);
            }
        };

        Object request = httpHandler.startRequest(url, callback);
        if (request != null) {
            httpRequestHandles.put(requestHandle, request);
        }
    }

    // Called from JNI on worker or render-thread.
    @Keep
    void sceneReadyCallback(final int sceneId, final int errorType, final String updatePath, final String updateValue) {
        final SceneLoadListener listener = sceneLoadListener;
        if (listener != null) {
            uiThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    SceneError error = null;
                    if (errorType >= 0) {
                        error = new SceneError(updatePath, updateValue, errorType);
                    }
                    listener.onSceneReady(sceneId, error);
                }
            });
        }
    }

    // Called from JNI on render-thread.
    @Keep
    void cameraAnimationCallback(final boolean finished) {
        final CameraAnimationCallback callback = cameraAnimationCallback;
        synchronized (cameraAnimationCallbackLock) {
            cameraAnimationCallback = pendingCameraAnimationCallback;
            pendingCameraAnimationCallback = null;
        }
        if (callback != null) {
            uiThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (finished) {
                        callback.onFinish();
                    } else {
                        callback.onCancel();
                    }
                }
            });
        }
    }

    @Keep
    void featurePickCallback(final Map<String, String> properties, final float x, final float y) {
        final FeaturePickListener listener = featurePickListener;
        if (listener != null) {
            uiThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    FeaturePickResult result = null;
                    if (properties != null) {
                        result = new FeaturePickResult(properties, x, y);
                    }
                    listener.onFeaturePickComplete(result);
                }
            });
        }
    }

    @Keep
    void labelPickCallback(final Map<String, String> properties, final float x, final float y, final int type, final double lng, final double lat) {
        final LabelPickListener listener = labelPickListener;
        if (listener != null) {
            uiThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    LabelPickResult result = null;
                    if (properties != null) {
                        result = new LabelPickResult(properties, lng, lat, x, y, type);
                    }
                    listener.onLabelPickComplete(result);
                }
            });
        }
    }

    @Keep
    void markerPickCallback(final long markerId, final float x, final float y, final double lng, final double lat) {
        final MarkerPickListener listener = markerPickListener;
        if (listener != null) {
            uiThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    final Marker marker = markers.get(markerId);
                    MarkerPickResult result = null;
                    if (marker != null) {
                        result = new MarkerPickResult(marker, lng, lat, x, y);
                    }
                    listener.onMarkerPickComplete(result);
                }
            });
        }
    }

    // Font Fetching
    // =============
    @Keep
    String getFontFilePath(final String key) {
        return FontConfig.getFontFile(key);
    }

    @Keep
    String getFontFallbackFilePath(final int importance, final int weightHint) {
        return FontConfig.getFontFallback(importance, weightHint);
    }

    // Native map
    // ==========

    NativeMap nativeMap;
    MapChangeListener mapChangeListener;

    // Private members
    // ===============

    private MapRenderer mapRenderer;
    private GLViewHolder viewHolder;
    private MapRegionChangeState currentState = MapRegionChangeState.IDLE;
    private TouchInput touchInput;
    private HttpHandler httpHandler;
    private final Map<Long, Object> httpRequestHandles = Collections.synchronizedMap(new HashMap<Long, Object>());
    private FeaturePickListener featurePickListener;
    private SceneLoadListener sceneLoadListener;
    private LabelPickListener labelPickListener;
    private MarkerPickListener markerPickListener;
    private Map<String, MapData> clientTileSources;
    private LongSparseArray<Marker> markers;
    private Handler uiThreadHandler;
    private CameraAnimationCallback cameraAnimationCallback;
    private CameraAnimationCallback pendingCameraAnimationCallback;
    private final Object cameraAnimationCallbackLock = new Object();
}
