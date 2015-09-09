package com.mapzen.tangram.android;

import android.app.Activity;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;

import com.mapzen.tangram.LngLat;
import com.mapzen.tangram.MapController;
import com.mapzen.tangram.MapData;
import com.mapzen.tangram.MapView;

import java.io.File;
import java.util.Arrays;

public class MainActivity extends Activity {

    MapController mapController;
    MapView mapView;
    MapData touchMarkers;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);

        setContentView(R.layout.main);

        mapView = (MapView)findViewById(R.id.map);
        mapController = new MapController(this, mapView);
        mapController.setMapZoom(16);
        mapController.setMapPosition(-74.00976419448854, 40.70532700869127);

        final LngLat lastTappedPoint = new LngLat();

        mapController.setTapGestureListener(new View.OnGenericMotionListener() {
            @Override
            public boolean onGenericMotion(View v, MotionEvent event) {
                LngLat tapPoint = mapController.coordinatesAtScreenPosition(event.getX(), event.getY());
                if (touchMarkers == null) { touchMarkers = new MapData("touch"); }
                if (lastTappedPoint.longitude != 0 && lastTappedPoint.latitude != 0) {
                    touchMarkers.addLine(Arrays.asList(tapPoint, lastTappedPoint));
                }
                lastTappedPoint.set(tapPoint);
                return true;
            }
        });

        mapController.setLongPressListener(new View.OnGenericMotionListener() {
            @Override
            public boolean onGenericMotion(View v, MotionEvent event) {
                if (touchMarkers != null) { touchMarkers.clear(); }
                return true;
            }
        });

        try {
            File cacheDir = new File(getExternalCacheDir().getAbsolutePath() + "/tile_cache");
            mapController.setTileCache(cacheDir, 30 * 1024 * 1024);
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

}

