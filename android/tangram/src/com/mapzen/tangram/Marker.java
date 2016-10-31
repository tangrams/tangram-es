package com.mapzen.tangram;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.util.DisplayMetrics;

import com.mapzen.tangram.geometry.Polygon;
import com.mapzen.tangram.geometry.Polyline;

public class Marker {

    Context context;
    long pointer = 0;
    MapController map;
    boolean visible = true;

    Marker(Context context, long pointer, MapController map) {
        this.context = context;
        this.pointer = pointer;
        this.map = map;
    }

    public boolean setStyling(String styleStr) {
        return map.setMarkerStyling(pointer, styleStr);
    }

    public boolean setDrawable(int drawableId) {
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inTargetDensity = DisplayMetrics.DENSITY_DEFAULT;
        Bitmap bitmap = BitmapFactory.decodeResource(context.getResources(), drawableId, options);
        return setBitmap(bitmap);
    }

    public boolean setDrawable(Drawable drawable) {
        BitmapDrawable bitmapDrawable = (BitmapDrawable) drawable;
        bitmapDrawable.setTargetDensity(DisplayMetrics.DENSITY_DEFAULT);
        Bitmap bitmap = bitmapDrawable.getBitmap();
        return setBitmap(bitmap);
    }

    private boolean setBitmap(Bitmap bitmap) {
        int width = bitmap.getScaledWidth(DisplayMetrics.DENSITY_DEFAULT);
        int height = bitmap.getScaledHeight(DisplayMetrics.DENSITY_DEFAULT);

        int[] data = new int[width * height];
        bitmap.getPixels(data, 0, width, 0, 0, width, height);

        return map.setMarkerBitmap(pointer, width, height, data);
    }

    public boolean setPoint(LngLat point) {
        return map.setMarkerPoint(pointer, point.longitude, point.latitude);
    }

    public boolean setPointEased(LngLat point, float duration, MapController.EaseType ease) {
        if (point == null) {
            return false;
        }
        return map.setMarkerPointEased(pointer, point.longitude, point.latitude, duration, ease);
    }

    public boolean setPolyline(Polyline polyline) {
        if (polyline == null) {
            return false;
        }
        return map.setMarkerPolyline(pointer, polyline.getCoordinateArray(),
                polyline.getCoordinateArray().length/2);
    }

    public boolean setPolygon(Polygon polygon) {
        if (polygon == null) {
            return false;
        }
        return map.setMarkerPolygon(pointer, polygon.getCoordinateArray(),
                polygon.getRingArray(), polygon.getRingArray().length);
    }

    public boolean setVisible(boolean visible) {
        boolean success = map.setMarkerVisible(pointer, visible);
        if (success) {
            this.visible = visible;
        }
        return success;
    }

    public boolean setDrawOrder(int drawOrder) {
        return map.setMarkerDrawOrder(pointer, drawOrder);
    }

    public boolean isVisible() {
        return visible;
    }
}
