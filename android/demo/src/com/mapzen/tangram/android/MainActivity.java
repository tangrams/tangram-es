package com.mapzen.tangram.android;

import android.app.Activity;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.widget.Toast;

import com.mapzen.tangram.HttpHandler;
import com.mapzen.tangram.LngLat;
import com.mapzen.tangram.MapController;
import com.mapzen.tangram.MapData;
import com.mapzen.tangram.MapView;
import com.mapzen.tangram.Properties;
import com.mapzen.tangram.tangram;
import com.mapzen.tangram.ClientGeoJsonSource;
import com.mapzen.tangram.Tags;

import com.squareup.okhttp.Callback;

import java.io.File;
import java.util.Arrays;
import java.lang.Math;

public class MainActivity extends Activity {

    MapController mapController;
    MapView mapView;
    MapData touchMarkers;

    String tileApiKey = "?api_key=vector-tiles-tyHL4AY";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);

        setContentView(R.layout.main);

        mapView = (MapView)findViewById(R.id.map);
        mapController = new MapController(this, mapView);
        mapController.setMapZoom(16);
        mapController.setMapPosition(-74.00976419448854, 40.70532700869127);

        final ClientGeoJsonSource touchMarkers = new ClientGeoJsonSource("touch", "");
        tangram.addDataSource(touchMarkers);
        final Tags tags = new Tags();

        final LngLat lastTappedPoint = new LngLat();
        final String colors[] = {"blue", "red", "green" };
        mapController.setTapGestureListener(new View.OnGenericMotionListener() {
            @Override
            public boolean onGenericMotion(View v, MotionEvent event) {
                LngLat tapPoint = mapController.coordinatesAtScreenPosition(event.getX(), event.getY());

                if (lastTappedPoint.longitude != 0 && lastTappedPoint.latitude != 0) {
                    tags.set("color", colors[(int)(Math.random() * 2.0 + 0.5)] );
                    touchMarkers.addLine(tags, new double[] {
                            tapPoint.longitude, tapPoint.latitude,
                            lastTappedPoint.longitude, lastTappedPoint.latitude}, 2);

                    touchMarkers.update();
                }
                lastTappedPoint.set(tapPoint);

                mapController.pickFeature(event.getX(), event.getY());
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

        mapController.setGenericMotionEventListener(new View.OnGenericMotionListener() {
            @Override
            public boolean onGenericMotion(View v, MotionEvent event) {
                // Handle generic motion event.
                return false;
            }
        });

        mapController.setFeatureTouchListener(new MapController.FeatureTouchListener() {
            @Override
            public void onTouch(Properties properties) {
                String name = properties.getString("name");
                if (name.length() == 0) {
                    name = "unnamed...";
                }
                Toast.makeText(getApplicationContext(),
                        "Selected: " + name,
                        Toast.LENGTH_SHORT).show();
            }
        });

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

    }

}

