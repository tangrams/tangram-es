package com.mapzen.tangram.android;

import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.Window;

import com.google.vrtoolkit.cardboard.CardboardActivity;
import com.mapzen.tangram.HttpHandler;
import com.mapzen.tangram.MapController;
import com.mapzen.tangram.MapData;
import com.mapzen.tangram.MapView;
import com.mapzen.tangram.TouchInput;
import com.squareup.okhttp.Callback;

import java.io.File;

public class MainActivity extends CardboardActivity {

    MapController mapController;
    MapView mapView;

    String tileApiKey = "?api_key=vector-tiles-tyHL4AY";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);

        setContentView(R.layout.main);

        mapView = (MapView)findViewById(R.id.map);
        mapController = new MapController(this, mapView);
        mapController.setMapZoom(18);
        mapController.setMapPosition(-74.00976419448854, 40.70532700869127);

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

    }

}

