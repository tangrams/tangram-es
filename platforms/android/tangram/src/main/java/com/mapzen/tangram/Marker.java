package com.mapzen.tangram;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.support.annotation.Keep;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.DisplayMetrics;

import com.mapzen.tangram.geometry.Polygon;
import com.mapzen.tangram.geometry.Polyline;

/**
 * Class used to display points, polylines, and bitmaps dynamically on a map. Do not create one of
 * these objects directly, instead use {@link MapController#addMarker()}.
 */
@Keep
public class Marker {

    private final Context context;
    private long markerId = 0;
    private final MapController map;
    private boolean visible = true;
    private Object userData = null;

    /**
     * Package private constructor for creating a new {@link Marker}.
     * @param context the context to use for decoding bitmap resources
     * @param markerId the marker id
     * @param map the map this marker is added to
     */
    Marker(@NonNull final Context context, final long markerId, @NonNull final MapController map) {
        this.context = context;
        this.markerId = markerId;
        this.map = map;
    }

    /**
     * Custom user data storage.
     * @param userData The user data to hold in this marker.
     */
    public void setUserData(@Nullable final Object userData) {
        this.userData = userData;
    }

    /**
     * Gets custom user data.
     * @return The user data held by this marker.
     */
    @Nullable
    public Object getUserData() {
        return this.userData;
    }

    /**
     * Returns the {@link Marker} id
     * @return marker id
     */
    public long getMarkerId() {
        return markerId;
    }

    /**
     * Used to style the marker
     * Sets draw rules from a draw group of a scene layer in the scene file, used to load the scene.
     * This draw group must be defined in the loaded scene file.
     *
     * <ul>
     * <li>layers.layer_a.layer_b.draw.some_draw_rule</li>
     * <li>layers.layer_c.draw.another_draw_rule</li>
     * </ul>
     *
     * @param path Absolute path to a draw rule in the current scene, delimited with "."
     * @return whether the styling was successfully set on the marker.
     */
    public boolean setStylingFromPath(final String path) {
        return map.setMarkerStylingFromPath(markerId, path);
    }

    /**
     * Used to style the marker
     * Sets the styling to be used to display either a point, polyline, or bitmap for this marker.
     * If the marker is going to be used to display a bitmap, a 'points' style must be set.
     *
     * <ul>
     * <li>{ style: 'points', color: 'white', size: [50px, 50px], order: 2000, collide: false }</li>
     * <li>{ style: 'lines', color: '#06a6d4', width: 5px, order: 2000 }</li>
     * <li>{ style: 'polygons', color: '#06a6d4', width: 5px, order: 2000 }</li>
     * </ul>
     *
     * @param styleString the style string
     * @return whether the style was successfully set
     */
    public boolean setStylingFromString(final String styleString) {
        return map.setMarkerStylingFromString(markerId, styleString);
    }

    /**
     * Sets the drawable resource id to be used to load a bitmap. When displaying a drawable, a
     * 'points' style must also be set on the marker (see {@link Marker#setStylingFromString(String)}.
     *
     * @param drawableId the drawable resource id
     * @return whether the drawable's bitmap was successfully set
     */
    public boolean setDrawable(final int drawableId) {
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inTargetDensity = context.getResources().getDisplayMetrics().densityDpi;
        Bitmap bitmap = BitmapFactory.decodeResource(context.getResources(), drawableId, options);
        return setBitmap(bitmap);
    }

    /**
     * Sets the drawable to be used to load a bitmap. When displaying a drawable, a
     * 'points' style must also be set on the marker (see {@link Marker#setStylingFromString(String)}.
     *
     * @param drawable the drawable
     * @return whether the drawable's bitmap was successfully set
     */
    public boolean setDrawable(@NonNull final Drawable drawable) {
        int density = context.getResources().getDisplayMetrics().densityDpi;
        BitmapDrawable bitmapDrawable = (BitmapDrawable) drawable;
        bitmapDrawable.setTargetDensity(density);
        Bitmap bitmap = bitmapDrawable.getBitmap();
        bitmap.setDensity(density);
        return setBitmap(bitmap);
    }

    /**
     * Sets the coordinate location, where the marker should be displayed.
     * @param point lat/lng location
     * @return whether the point was successfully set
     */
    public boolean setPoint(@NonNull final LngLat point) {
        return map.setMarkerPoint(markerId, point.longitude, point.latitude);
    }

    /**
     * Sets the coordinate location, where the marker should be displayed with animation.
     * @param point lat/lng location
     * @param duration animation duration in milliseconds
     * @param ease animation type
     * @return whether the point was successfully set
     */
    public boolean setPointEased(@Nullable final LngLat point, final int duration,
                                 @NonNull final MapController.EaseType ease) {
        if (point == null) {
            return false;
        }
        return map.setMarkerPointEased(markerId, point.longitude, point.latitude, duration, ease);
    }

    /**
     * Sets the polyline to be displayed. When using this method, a 'polyline' style must also be
     * set. See {@link Marker#setStylingFromString(String)}.
     * @param polyline the polyline to display
     * @return whether the polyline was successfully set
     */
    public boolean setPolyline(@Nullable final Polyline polyline) {
        if (polyline == null) {
            return false;
        }
        return map.setMarkerPolyline(markerId, polyline.getCoordinateArray(),
                polyline.getCoordinateArray().length/2);
    }

    /**
     * Sets the polygon to be displayed. When using this method, a 'polygon' style must also be
     * set. See {@link Marker#setStylingFromString(String)}.
     * @param polygon the polygon to display
     * @return whether the polygon was successfully set
     */
    public boolean setPolygon(@Nullable final Polygon polygon) {
        if (polygon == null) {
            return false;
        }
        return map.setMarkerPolygon(markerId, polygon.getCoordinateArray(),
                polygon.getRingArray(), polygon.getRingArray().length);
    }

    /**
     * Changes the marker's visibility on the map.
     * @param visible whether or not the marker should be visible
     * @return whether the marker's visibility was successfully set
     */
    public boolean setVisible(final boolean visible) {
        boolean success = map.setMarkerVisible(markerId, visible);
        if (success) {
            this.visible = visible;
        }
        return success;
    }

    /**
     * Sets the draw order for the marker to be used in z-order collisions.
     * @param drawOrder the draw order to set
     * @return whether the marker's draw order was successfully set
     */
    public boolean setDrawOrder(final int drawOrder) {
        return map.setMarkerDrawOrder(markerId, drawOrder);
    }

    /**
     * Returns whether or not the marker is visible on the map.
     * @return whether the marker is visible on the map
     */
    public boolean isVisible() {
        return visible;
    }

    private boolean setBitmap(@NonNull final Bitmap bitmap) {
        float density = context.getResources().getDisplayMetrics().density;
        return map.setMarkerBitmap(markerId, bitmap, density);
    }

    void invalidate() {
        this.markerId = 0;
    }
}
