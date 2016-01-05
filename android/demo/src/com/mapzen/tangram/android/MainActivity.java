package com.mapzen.tangram.android;

import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.Window;

import com.google.vrtoolkit.cardboard.CardboardActivity;
import com.mapzen.tangram.HttpHandler;
import com.mapzen.tangram.MapController;
import com.mapzen.tangram.MapView;
import com.mapzen.tangram.TouchInput;
import com.squareup.okhttp.Callback;

import java.io.File;

public class MainActivity extends CardboardActivity {

    MapController mapController;
    MapView mapView;

    String tileApiKey = "?api_key=vector-tiles-tyHL4AY";

    int locationIndex = 0;
    double[] locationCoordinates = {
            -74.00976419448854, 40.70532700869127, // Manhattan
            -122.39901, 37.79241, // San Francisco
            -0.11870, 51.50721, // London
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);

        setContentView(R.layout.main);

        mapView = (MapView)findViewById(R.id.map);
        mapController = new MapController(this, mapView);
        mapController.setMapZoom(18);

        goToLocation(locationIndex);

        HttpHandler handler = new HttpHandler() {
            @Override
            public boolean onRequest(String url, Callback cb) {
                url += tileApiKey;
                return super.onRequest(url, cb);
            }

            @Override
            public void onCancel(String url) {
                url += tileApiKey;
                super.onCancel(url);
            }
        };

        try {
            File httpCache = new File(getExternalCacheDir().getAbsolutePath() + "/tile_cache");
            handler.setCache(httpCache, 30 * 1024 * 1024);
        } catch (Exception e) {
            e.printStackTrace();
        }

        mapController.setHttpHandler(handler);

        mapController.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);

        setCardboardView(mapView);
        setConvertTapIntoTrigger(true);

    }

    @Override
    public void onCardboardTrigger() {
        locationIndex++;
        goToLocation(locationIndex);
    }

    void goToLocation(int index) {

        index %= locationCoordinates.length / 2;

        double lon = locationCoordinates[2 * index];
        double lat = locationCoordinates[2 * index + 1];

        mapController.setMapPosition(lon, lat);

    }

}

