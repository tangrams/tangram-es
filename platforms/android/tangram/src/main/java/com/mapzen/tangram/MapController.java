package com.mapzen.tangram;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.PointF;
import android.graphics.Rect;
import android.os.Build;
import android.os.Handler;
import android.support.annotation.IntRange;
import android.support.annotation.Keep;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
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
     * Triggered after a call of {@link #updateSceneAsync(List<SceneUpdate>)} or
     * {@link #loadSceneFileAsync(String, List<SceneUpdate>)} or {@link #loadSceneFile(String, List<SceneUpdate>)}
     * Listener should be set with {@link #setSceneLoadListener(SceneLoadListener)}
     * The callbacks will be run on the main (UI) thread.
     */
    @Keep
    public interface SceneLoadListener {
        /**
         * Received when a scene load or update finishes. If sceneError is not null then the operation did not succeed.
         * @param sceneId The identifier returned by {@link #updateSceneAsync(List<SceneUpdate>)} or
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
        displayMetrics = context.getResources().getDisplayMetrics();
        assetManager = context.getAssets();

        // Parse font file description
        //FontConfig.init();

        mapPointer = nativeInit(assetManager);
        if (mapPointer <= 0) {
            throw new RuntimeException("Unable to create a native Map object! There may be insufficient memory available.");
        }
        nativeSetPixelScale(mapPointer, displayMetrics.density);
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
        isGLRendererSet = true;
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
        if (mapPointer == 0) { return; }

        Log.e("TANGRAM", ">>> dispose");
        nativeShutdown(mapPointer);
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
        frameCaptureCallback = null;

        // Prevent any calls to native functions - except dispose.
        final long pointer = mapPointer;
        mapPointer = 0;

        // NOTE: It is possible for the MapView held by a ViewGroup to be removed, calling detachFromWindow which
        // stops the Render Thread associated with GLSurfaceView, possibly resulting in leaks from render thread
        // queue. Because of the above, destruction lifecycle of the GLSurfaceView will not be triggered.
        // To avoid the above senario, nativeDispose is called from the UIThread.
        //
        // Since all gl resources will be freed when GLSurfaceView is deleted this is safe until
        // we support sharing gl contexts.
        nativeDispose(pointer);
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
        checkPointer(mapPointer);
        final String[] updateStrings = bundleSceneUpdates(sceneUpdates);
        final int sceneId = nativeLoadScene(mapPointer, path, updateStrings);
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
        checkPointer(mapPointer);
        final String[] updateStrings = bundleSceneUpdates(sceneUpdates);
        final int sceneId = nativeLoadSceneAsync(mapPointer, path, updateStrings);
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
        checkPointer(mapPointer);
        final String[] updateStrings = bundleSceneUpdates(sceneUpdates);
        final int sceneId = nativeLoadSceneYaml(mapPointer, yaml, resourceRoot, updateStrings);
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
        checkPointer(mapPointer);
        final String[] updateStrings = bundleSceneUpdates(sceneUpdates);
        final int sceneId = nativeLoadSceneYamlAsync(mapPointer, yaml, resourceRoot, updateStrings);
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
        checkPointer(mapPointer);
        if (duration > 0) {
            setMapRegionState(MapRegionChangeState.ANIMATING);
        } else {
            setMapRegionState(MapRegionChangeState.JUMPING);
        }
        setPendingCameraAnimationCallback(cb);
        final float seconds = duration / 1000.f;
        nativeUpdateCameraPosition(mapPointer, update.set, update.longitude, update.latitude, update.zoom,
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
        checkPointer(mapPointer);
        // TODO: Make sense to have mark animating for flyTo irrespective of duration/speed?
        setMapRegionState(MapRegionChangeState.ANIMATING);
        setPendingCameraAnimationCallback(callback);
        final float seconds = duration / 1000.f;
        nativeFlyTo(mapPointer, position.longitude, position.latitude, position.zoom, seconds, speed);
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
        checkPointer(mapPointer);
        final double[] pos = { 0, 0 };
        final float[] zrt = { 0, 0, 0 };
        nativeGetCameraPosition(mapPointer, pos, zrt);
        out.longitude = pos[0];
        out.latitude = pos[1];
        out.zoom = zrt[0];
        out.rotation = zrt[1];
        out.tilt = zrt[2];
        return out;
    }

    /**
     * Cancel current camera animation
     */
    public void cancelCameraAnimation() {
        checkPointer(mapPointer);
        nativeCancelCameraAnimation(mapPointer);
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
        int pad[] = new int[]{padding.left, padding.top, padding.right, padding.bottom};
        double lngLatZoom[] = new double[3];
        nativeGetEnclosingCameraPosition(mapPointer, sw.longitude, sw.latitude, ne.longitude, ne.latitude, pad, lngLatZoom);
        out.longitude = lngLatZoom[0];
        out.latitude = lngLatZoom[1];
        out.zoom = (float)lngLatZoom[2];
        out.rotation = 0.f;
        out.tilt = 0.f;
        return out;
    }

    /**
     * Set the camera type for the map view
     * @param type A {@code CameraType}
     */
    public void setCameraType(@NonNull final CameraType type) {
        checkPointer(mapPointer);
        nativeSetCameraType(mapPointer, type.ordinal());
    }

    /**
     * Get the camera type currently in use for the map view
     * @return A {@code CameraType}
     */
    public CameraType getCameraType() {
        checkPointer(mapPointer);
        return CameraType.values()[nativeGetCameraType(mapPointer)];
    }

    /**
     * Get the minimum zoom level of the map view. The default minimum zoom is 0.
     * @return The zoom level
     */
    public float getMinimumZoomLevel() {
        checkPointer(mapPointer);
        return nativeGetMinZoom(mapPointer);
    }

    /**
     * Set the minimum zoom level of the map view.
     *
     * Values less than the default minimum zoom will be clamped. Assigning a value greater than the
     * current maximum zoom will set the maximum zoom to this value.
     * @param minimumZoom The zoom level
     */
    public void setMinimumZoomLevel(float minimumZoom) {
        checkPointer(mapPointer);
        nativeSetMinZoom(mapPointer, minimumZoom);
    }

    /**
     * Get the maximum zoom level of the map view. The default maximum zoom is 20.5.
     * @return The zoom level
     */
    public float getMaximumZoomLevel() {
        checkPointer(mapPointer);
        return nativeGetMaxZoom(mapPointer);
    }

    /**
     * Set the maximum zoom level of the map view.
     *
     * Values greater than the default maximum zoom will be clamped. Assigning a value less than the
     * current minimum zoom will set the minimum zoom to this value.
     * @param maximumZoom The zoom level
     */
    public void setMaximumZoomLevel(float maximumZoom) {
        checkPointer(mapPointer);
        nativeSetMaxZoom(mapPointer, maximumZoom);
    }

    /**
     * Find the geographic coordinates corresponding to the given position on screen
     * @param screenPosition Position in pixels from the top-left corner of the map area
     * @return LngLat corresponding to the given point, or null if the screen position
     * does not intersect a geographic location (this can happen at high tilt angles).
     */
    @Nullable
    public LngLat screenPositionToLngLat(@NonNull final PointF screenPosition) {
        checkPointer(mapPointer);
        final double[] tmp = { screenPosition.x, screenPosition.y };
        if (nativeScreenPositionToLngLat(mapPointer, tmp)) {
            return new LngLat(tmp[0], tmp[1]);
        }
        return null;
    }

    /**
     * Find the position on screen corresponding to the given geographic coordinates
     * @param lngLat Geographic coordinates
     * @return Position in pixels from the top-left corner of the map area (the point
     * may not lie within the viewable screen area)
     */
    @NonNull
    public PointF lngLatToScreenPosition(@NonNull final LngLat lngLat) {
        checkPointer(mapPointer);
        final double[] tmp = { lngLat.longitude, lngLat.latitude };
        nativeLngLatToScreenPosition(mapPointer, tmp);
        return new PointF((float)tmp[0], (float)tmp[1]);
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
        checkPointer(mapPointer);
        final long pointer = nativeAddTileSource(mapPointer, name, generateCentroid);
        if (pointer <= 0) {
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
        checkPointer(mapPointer);
        checkPointer(mapData.pointer);
        nativeRemoveTileSource(mapPointer, mapData.pointer);
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
                nativeHandlePanGesture(mapPointer, startX, startY, endX, endY);
                return true;
            }

            @Override
            public boolean onPanEnd() {
                setMapRegionState(MapRegionChangeState.IDLE);
                return true;
            }

            @Override
            public boolean onFling(final float posX, final float posY, final float velocityX, final float velocityY) {
                nativeHandleFlingGesture(mapPointer, posX, posY, velocityX, velocityY);
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
                nativeHandleRotateGesture(mapPointer, x, y, rotation);
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
                nativeHandlePinchGesture(mapPointer, x, y, scale, velocity);
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
                nativeHandleShoveGesture(mapPointer, distance);
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
        checkPointer(mapPointer);
        nativeSetPickRadius(mapPointer, radius);
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
            public void onLabelPick(final LabelPickResult labelPickResult, final float positionX, final float positionY) {
                uiThreadHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        listener.onLabelPick(labelPickResult, positionX, positionY);
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
            public void onMarkerPick(final MarkerPickResult markerPickResult, final float positionX, final float positionY) {
                uiThreadHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        listener.onMarkerPick(markerPickResult, positionX, positionY);
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
            checkPointer(mapPointer);
            nativePickFeature(mapPointer, posX, posY);
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
            checkPointer(mapPointer);
            nativePickLabel(mapPointer, posX, posY);
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
            checkPointer(mapPointer);
            nativePickMarker(mapPointer, posX, posY);
        }
    }

    /**
     * Adds a {@link Marker} to the map which can be used to dynamically add points and polylines
     * to the map.
     * @return Newly created {@link Marker} object.
     */
    @NonNull
    public Marker addMarker() {
        checkPointer(mapPointer);
        final long markerId = nativeMarkerAdd(mapPointer);

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
        checkPointer(mapPointer);
        checkId(markerId);
        markers.remove(markerId);
        return nativeMarkerRemove(mapPointer, markerId);
    }

    /**
     * Remove all the {@link Marker} objects from the map.
     */
    public void removeAllMarkers() {
        checkPointer(mapPointer);
        nativeMarkerRemoveAll(mapPointer);

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
        nativeSetDebugFlag(flag.ordinal(), on);
    }

    /**
     * Set whether the OpenGL state will be cached between subsequent frames. This improves
     * rendering efficiency, but can cause errors if your application code makes OpenGL calls.
     * @param use Whether to use a cached OpenGL state; false by default
     */
    public void useCachedGlState(final boolean use) {
        checkPointer(mapPointer);
        nativeUseCachedGlState(mapPointer, use);
    }

    /**
     * Sets an opaque background color used as default color when a scene is being loaded
     * @param red red component of the background color
     * @param green green component of the background color
     * @param blue blue component of the background color
     */
    public void setDefaultBackgroundColor(final float red, final float green, final float blue) {
        checkPointer(mapPointer);
        nativeSetDefaultBackgroundColor(mapPointer, red, green, blue);
    }

    // Package private methods
    // =======================

    void onLowMemory() {
        checkPointer(mapPointer);
        nativeOnLowMemory(mapPointer);
    }

    void removeTileSource(final long sourcePtr) {
        checkPointer(mapPointer);
        checkPointer(sourcePtr);
        nativeRemoveTileSource(mapPointer, sourcePtr);
    }

    void clearTileSource(final long sourcePtr) {
        checkPointer(mapPointer);
        checkPointer(sourcePtr);
        nativeClearTileSource(mapPointer, sourcePtr);
    }


    void checkPointer(final long ptr) {
        if (ptr <= 0) {
            throw new RuntimeException("Tried to perform an operation on an invalid pointer!"
                    + " This means you may have used an object that has been disposed and is no"
                    + " longer valid.");
        }
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
        checkPointer(mapPointer);
        checkId(markerId);
        return nativeMarkerSetStylingFromString(mapPointer, markerId, styleString);
    }

    boolean setMarkerStylingFromPath(final long markerId, final String path) {
        checkPointer(mapPointer);
        checkId(markerId);
        return nativeMarkerSetStylingFromPath(mapPointer, markerId, path);
    }

    boolean setMarkerBitmap(final long markerId, Bitmap bitmap, float density) {
        checkPointer(mapPointer);
        checkId(markerId);
        return nativeMarkerSetBitmap(mapPointer, markerId, bitmap, density);
    }

    boolean setMarkerPoint(final long markerId, final double lng, final double lat) {
        checkPointer(mapPointer);
        checkId(markerId);
        return nativeMarkerSetPoint(mapPointer, markerId, lng, lat);
    }

    boolean setMarkerPointEased(final long markerId, final double lng, final double lat, final int duration,
                                @NonNull final EaseType ease) {
        checkPointer(mapPointer);
        checkId(markerId);
        final float seconds = duration / 1000.f;
        return nativeMarkerSetPointEased(mapPointer, markerId, lng, lat, seconds, ease.ordinal());
    }

    boolean setMarkerPolyline(final long markerId, final double[] coordinates, final int count) {
        checkPointer(mapPointer);
        checkId(markerId);
        return nativeMarkerSetPolyline(mapPointer, markerId, coordinates, count);
    }

    boolean setMarkerPolygon(final long markerId, final double[] coordinates, final int[] rings, final int count) {
        checkPointer(mapPointer);
        checkId(markerId);
        return nativeMarkerSetPolygon(mapPointer, markerId, coordinates, rings, count);
    }

    boolean setMarkerVisible(final long markerId, final boolean visible) {
        checkPointer(mapPointer);
        checkId(markerId);
        return nativeMarkerSetVisible(mapPointer, markerId, visible);
    }

    boolean setMarkerDrawOrder(final long markerId, final int drawOrder) {
        checkPointer(mapPointer);
        checkId(markerId);
        return nativeMarkerSetDrawOrder(mapPointer, markerId, drawOrder);
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
                nativeOnUrlComplete(mapPointer, requestHandle, null, msg);
            }

            @Override
            public void onResponse(final int code, @Nullable final byte[] rawDataBytes) {
                if (httpRequestHandles.remove(requestHandle) == null) { return; }
                if (code >= 200 && code < 300) {
                    nativeOnUrlComplete(mapPointer, requestHandle, rawDataBytes, null);
                } else {
                    nativeOnUrlComplete(mapPointer, requestHandle, null,
                            "Unexpected response code: " + code + " for URL: " + url);
                }
            }

            @Override
            public void onCancel() {
                if (httpRequestHandles.remove(requestHandle) == null) { return; }
                nativeOnUrlComplete(mapPointer, requestHandle, null, null);
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
                    listener.onFeaturePick(properties, x, y);
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
                        result = new LabelPickResult(lng, lat, type, properties);
                    }
                    listener.onLabelPick(result, x, y);
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
                        result = new MarkerPickResult(marker, lng, lat);
                    }
                    listener.onMarkerPick(result, x, y);
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

    // Private members
    // ===============

    long mapPointer;
    private MapRenderer mapRenderer;
    private GLViewHolder viewHolder;
    private MapRegionChangeState currentState = MapRegionChangeState.IDLE;
    private AssetManager assetManager;
    private TouchInput touchInput;
    private DisplayMetrics displayMetrics = new DisplayMetrics();
    private HttpHandler httpHandler;
    private final Map<Long, Object> httpRequestHandles = Collections.synchronizedMap(new HashMap());
    MapChangeListener mapChangeListener;
    private FeaturePickListener featurePickListener;
    private SceneLoadListener sceneLoadListener;
    private LabelPickListener labelPickListener;
    private MarkerPickListener markerPickListener;
    private FrameCaptureCallback frameCaptureCallback;
    private boolean frameCaptureAwaitCompleteView;
    private Map<String, MapData> clientTileSources;
    private LongSparseArray<Marker> markers;
    private Handler uiThreadHandler;
    private CameraAnimationCallback cameraAnimationCallback;
    private CameraAnimationCallback pendingCameraAnimationCallback;
    private final Object cameraAnimationCallbackLock = new Object();
    private boolean isGLRendererSet = false;

    // Native methods
    // ==============

    private synchronized native void nativeOnLowMemory(long mapPtr);
    private synchronized native long nativeInit(AssetManager assetManager);
    private synchronized native void nativeDispose(long mapPtr);
    private synchronized native void nativeShutdown(long mapPtr);
    private synchronized native int nativeLoadScene(long mapPtr, String path, String[] updateStrings);
    private synchronized native int nativeLoadSceneAsync(long mapPtr, String path, String[] updateStrings);
    private synchronized native int nativeLoadSceneYaml(long mapPtr, String yaml, String resourceRoot, String[] updateStrings);
    private synchronized native int nativeLoadSceneYamlAsync(long mapPtr, String yaml, String resourceRoot, String[] updateStrings);

    private synchronized native void nativeGetCameraPosition(long mapPtr, double[] lonLatOut, float[] zoomRotationTiltOut);
    private synchronized native void nativeUpdateCameraPosition(long mapPtr, int set, double lon, double lat, float zoom, float zoomBy,
                                                                float rotation, float rotateBy, float tilt, float tiltBy,
                                                                double b1lon, double b1lat, double b2lon, double b2lat, int[] padding,
                                                                float duration, int ease);
    private synchronized native void nativeFlyTo(long mapPtr, double lon, double lat, float zoom, float duration, float speed);
    private synchronized native void nativeGetEnclosingCameraPosition(long mapPtr, double aLng, double aLat, double bLng, double bLat, int[] buffer, double[] lngLatZoom);
    private synchronized native void nativeCancelCameraAnimation(long mapPtr);
    private synchronized native boolean nativeScreenPositionToLngLat(long mapPtr, double[] coordinates);
    private synchronized native boolean nativeLngLatToScreenPosition(long mapPtr, double[] coordinates);
    private synchronized native void nativeSetPixelScale(long mapPtr, float scale);
    private synchronized native void nativeSetCameraType(long mapPtr, int type);
    private synchronized native int nativeGetCameraType(long mapPtr);
    private synchronized native float nativeGetMinZoom(long mapPtr);
    private synchronized native void nativeSetMinZoom(long mapPtr, float minZoom);
    private synchronized native float nativeGetMaxZoom(long mapPtr);
    private synchronized native void nativeSetMaxZoom(long mapPtr, float maxZoom);
    private synchronized native void nativeHandleTapGesture(long mapPtr, float posX, float posY);
    private synchronized native void nativeHandleDoubleTapGesture(long mapPtr, float posX, float posY);
    private synchronized native void nativeHandlePanGesture(long mapPtr, float startX, float startY, float endX, float endY);
    private synchronized native void nativeHandleFlingGesture(long mapPtr, float posX, float posY, float velocityX, float velocityY);
    private synchronized native void nativeHandlePinchGesture(long mapPtr, float posX, float posY, float scale, float velocity);
    private synchronized native void nativeHandleRotateGesture(long mapPtr, float posX, float posY, float rotation);
    private synchronized native void nativeHandleShoveGesture(long mapPtr, float distance);
    private synchronized native void nativeSetPickRadius(long mapPtr, float radius);
    private synchronized native void nativePickFeature(long mapPtr, float posX, float posY);
    private synchronized native void nativePickLabel(long mapPtr, float posX, float posY);
    private synchronized native void nativePickMarker(long mapPtr, float posX, float posY);
    private synchronized native long nativeMarkerAdd(long mapPtr);
    private synchronized native boolean nativeMarkerRemove(long mapPtr, long markerID);
    private synchronized native boolean nativeMarkerSetStylingFromString(long mapPtr, long markerID, String styling);
    private synchronized native boolean nativeMarkerSetStylingFromPath(long mapPtr, long markerID, String path);
    private synchronized native boolean nativeMarkerSetBitmap(long mapPtr, long markerID, Bitmap bitmap, float density);
    private synchronized native boolean nativeMarkerSetPoint(long mapPtr, long markerID, double lng, double lat);
    private synchronized native boolean nativeMarkerSetPointEased(long mapPtr, long markerID, double lng, double lat, float duration, int ease);
    private synchronized native boolean nativeMarkerSetPolyline(long mapPtr, long markerID, double[] coordinates, int count);
    private synchronized native boolean nativeMarkerSetPolygon(long mapPtr, long markerID, double[] coordinates, int[] rings, int count);
    private synchronized native boolean nativeMarkerSetVisible(long mapPtr, long markerID, boolean visible);
    private synchronized native boolean nativeMarkerSetDrawOrder(long mapPtr, long markerID, int drawOrder);
    private synchronized native void nativeMarkerRemoveAll(long mapPtr);
    private synchronized native void nativeUseCachedGlState(long mapPtr, boolean use);
    private synchronized native void nativeSetDefaultBackgroundColor(long mapPtr, float r, float g, float b);

    private synchronized native long nativeAddTileSource(long mapPtr, String name, boolean generateCentroid);
    private synchronized native void nativeRemoveTileSource(long mapPtr, long sourcePtr);
    private synchronized native void nativeClearTileSource(long mapPtr, long sourcePtr);

    private synchronized native void nativeSetDebugFlag(int flag, boolean on);

    private native void nativeOnUrlComplete(long mapPtr, long requestHandle, byte[] rawDataBytes, String errorMessage);

}
