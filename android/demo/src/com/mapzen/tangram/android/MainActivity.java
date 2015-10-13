package com.mapzen.tangram.android;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.ContextMenu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.Toast;

import com.mapzen.tangram.DebugFlags;
import com.mapzen.tangram.HttpHandler;
import com.mapzen.tangram.LngLat;
import com.mapzen.tangram.MapController;
import com.mapzen.tangram.MapData;
import com.mapzen.tangram.MapView;
import com.mapzen.tangram.Properties;
import com.mapzen.tangram.Tangram;
import com.mapzen.tangram.Coordinates;
import com.mapzen.tangram.Polygon;

import com.squareup.okhttp.Callback;

import java.io.File;
import java.util.Arrays;
import java.util.LinkedList;
import java.lang.Math;

public class MainActivity extends Activity {

    private static final int CONTEXT_MENU_DEFAULT = 0;
    private static final int CONTEXT_MENU_BLUEPRINT = 1;
    private static final int CONTEXT_MENU_GOTHAM = 2;
    private static final int CONTEXT_MENU_PERICOLI = 3;
    private static final int CONTEXT_MENU_CROSSHATCH = 4;
    private static final int CONTEXT_MENU_ERASER = 5;
    private static final int CONTEXT_MENU_IKEDA = 6;

    private static final int CONTEXT_MENU_BLUEPRINT2 = 11;
    private static final int CONTEXT_MENU_GOTHAM2 = 12;
    private static final int CONTEXT_MENU_PERICOLI2 = 13;
    private static final int CONTEXT_MENU_CROSSHATCH2 = 14;
    private static final int CONTEXT_MENU_ERASER2 = 15;
    private static final int CONTEXT_MENU_IKEDA2 = 16;

    MapController mapController;
    MapView mapView;
    MapData touchMarkers;

    boolean tileInfo;

    String tileApiKey = ""; // "?api_key=vector-tiles-tyHL4AY";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);

        setContentView(R.layout.main);

        mapView = (MapView)findViewById(R.id.map);
        mapController = new MapController(this, mapView, "es2_ikeda.yaml");
        mapController.setMapZoom(16);
        mapController.setMapPosition(-74.00976419448854, 40.70532700869127);

        registerForContextMenu(mapView);

        mapController.setLongPressListener(new View.OnGenericMotionListener() {
            @Override
            public boolean onGenericMotion(View v, MotionEvent event) {
                openContextMenu(mapView);
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

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v,
                                    ContextMenu.ContextMenuInfo menuInfo) {
        super.onCreateContextMenu(menu, v, menuInfo);

        menu.add(0, CONTEXT_MENU_DEFAULT, 0, "TangramES");
        menu.add(0, CONTEXT_MENU_BLUEPRINT, 0, "Blueprint");
        menu.add(0, CONTEXT_MENU_BLUEPRINT2, 0, "Blueprint (opt)");
        menu.add(0, CONTEXT_MENU_GOTHAM, 0, "Gotham");
        menu.add(0, CONTEXT_MENU_GOTHAM2, 0, "Gotham (opt)");
        menu.add(0, CONTEXT_MENU_PERICOLI, 0, "Pericoli");
        menu.add(0, CONTEXT_MENU_PERICOLI2, 0, "Pericoli (opt)");
        menu.add(0, CONTEXT_MENU_CROSSHATCH, 0, "Crosshatch");
        menu.add(0, CONTEXT_MENU_CROSSHATCH2, 0, "Crosshatch (opt)");
        menu.add(0, CONTEXT_MENU_ERASER, 0, "Eraser Map");
        menu.add(0, CONTEXT_MENU_ERASER2, 0, "Eraser Map (opt)");
        menu.add(0, CONTEXT_MENU_IKEDA, 0, "Ikeda");
        menu.add(0, CONTEXT_MENU_IKEDA2, 0, "Ikeda (opt)");
    }

    @Override
    public boolean onMenuItemSelected(int featureId, MenuItem item) {

        switch (item.getItemId()) {
            case CONTEXT_MENU_DEFAULT:
                mapController.setScene("scene.yaml");
                return true;

            case CONTEXT_MENU_BLUEPRINT:
                mapController.setScene("blueprint.yaml");
                return true;

            case CONTEXT_MENU_GOTHAM:
                mapController.setScene("gotham.yaml");
                return true;

            case CONTEXT_MENU_PERICOLI:
                mapController.setScene("pericoli.yaml");
                return true;

            case CONTEXT_MENU_CROSSHATCH:
                mapController.setScene("crosshatch.yaml");
                return true;

            case CONTEXT_MENU_ERASER:
                mapController.setScene("eraser.yaml");
                return true;

            case CONTEXT_MENU_IKEDA:
                mapController.setScene("ikeda.yaml");
                return true;

            case CONTEXT_MENU_BLUEPRINT2:
                mapController.setScene("es2_blueprint.yaml");
                return true;

            case CONTEXT_MENU_GOTHAM2:
                mapController.setScene("es2_gotham.yaml");
                return true;

            case CONTEXT_MENU_PERICOLI2:
                mapController.setScene("es2_pericoli.yaml");
                return true;

            case CONTEXT_MENU_CROSSHATCH2:
                mapController.setScene("es2_crosshatch.yaml");
                return true;

            case CONTEXT_MENU_ERASER2:
                mapController.setScene("es2_eraser.yaml");
                return true;

            case CONTEXT_MENU_IKEDA2:
                mapController.setScene("es2_ikeda.yaml");
                return true;

        }
        return true;
    }

}

