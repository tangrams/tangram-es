package com.mapzen.tangram.android;

import android.app.Activity;
import android.os.Bundle;
import android.view.Window;

import android.view.View;
import android.view.MotionEvent;

import com.mapzen.tangram.MapController;
import com.mapzen.tangram.MapView;

import java.io.File;

public class MainActivity extends Activity {

    MapController mapController;
    MapView mapView;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);

        setContentView(R.layout.main);

        mapView = (MapView)findViewById(R.id.map);
        mapController = new MapController(this, mapView);
        mapController.setMapZoom(16);
        mapController.setMapPosition(-74.00976419448854, 40.70532700869127);

        mapController.setTapGestureListener(new View.OnGenericMotionListener() {
            @Override
            public boolean onGenericMotion(View v, MotionEvent event) {
                double[] tapCoord = mapController.coordinatesAtScreenPosition(event.getX(), event.getY());
                mapController.setMapPosition(tapCoord[0], tapCoord[1]);
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

