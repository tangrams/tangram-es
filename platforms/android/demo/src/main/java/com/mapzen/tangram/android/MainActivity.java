package com.mapzen.tangram.android;

import android.graphics.PointF;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Window;
import android.widget.Toast;

import com.mapzen.tangram.CachePolicy;
import com.mapzen.tangram.HttpHandler;
import com.mapzen.tangram.LabelPickResult;
import com.mapzen.tangram.LngLat;
import com.mapzen.tangram.MapController;
import com.mapzen.tangram.MapController.FeaturePickListener;
import com.mapzen.tangram.MapController.LabelPickListener;
import com.mapzen.tangram.MapController.MarkerPickListener;
import com.mapzen.tangram.MapController.ViewCompleteListener;
import com.mapzen.tangram.MapData;
import com.mapzen.tangram.MapView;
import com.mapzen.tangram.Marker;
import com.mapzen.tangram.MarkerPickResult;
import com.mapzen.tangram.SceneError;
import com.mapzen.tangram.SceneUpdate;
import com.mapzen.tangram.TouchInput.DoubleTapResponder;
import com.mapzen.tangram.TouchInput.LongPressResponder;
import com.mapzen.tangram.TouchInput.TapResponder;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

import okhttp3.CacheControl;
import okhttp3.HttpUrl;

public class MainActivity extends AppCompatActivity implements MapController.SceneLoadListener, TapResponder,
        DoubleTapResponder, LongPressResponder, FeaturePickListener, LabelPickListener, MarkerPickListener {

    private static final String NEXTZEN_API_KEY = BuildConfig.NEXTZEN_API_KEY;

    private static final String TAG = "TangramDemo";

    private static final String[] SCENE_PRESETS = {
            "asset:///scene.yaml",
            "https://www.nextzen.org/carto/bubble-wrap-style/9/bubble-wrap-style.zip",
            "https://www.nextzen.org/carto/refill-style/11/refill-style.zip",
            "https://www.nextzen.org/carto/walkabout-style/7/walkabout-style.zip",
            "https://www.nextzen.org/carto/tron-style/6/tron-style.zip",
            "https://www.nextzen.org/carto/cinnabar-style/9/cinnabar-style.zip"
    };

    private ArrayList<SceneUpdate> sceneUpdates = new ArrayList<>();

    MapController map;
    MapView view;
    LngLat lastTappedPoint;
    MapData markers;

    PresetSelectionTextView sceneSelector;

    String pointStylingPath = "layers.touch.point.draw.icons";
    ArrayList<Marker> pointMarkers = new ArrayList<Marker>();

    boolean showTileInfo = false;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);

        setContentView(R.layout.main);

        if (NEXTZEN_API_KEY.isEmpty() || NEXTZEN_API_KEY.equals("null")) {
            Log.w(TAG, "No API key found! Nextzen data sources require an API key.\n" +
                    "Sign up for a free key at https://developers.nextzen.org/ and set it\n" +
                    "in your local Gradle properties file (~/.gradle/gradle.properties)\n" +
                    "as 'nextzenApiKey=YOUR-API-KEY-HERE'");
        }

        // Create a scene update to apply our API key in the scene.
        sceneUpdates.add(new SceneUpdate("global.sdk_api_key", NEXTZEN_API_KEY));

        // Set up a text view to allow selecting preset and custom scene URLs.
        sceneSelector = (PresetSelectionTextView)findViewById(R.id.sceneSelector);
        sceneSelector.setText(SCENE_PRESETS[0]);
        sceneSelector.setPresetStrings(Arrays.asList(SCENE_PRESETS));
        sceneSelector.setOnSelectionListener(new PresetSelectionTextView.OnSelectionListener() {
            @Override
            public void onSelection(String selection) {
                map.loadSceneFile(selection, sceneUpdates);
            }
        });

        // Grab a reference to our map view.
        view = (MapView)findViewById(R.id.map);
    }

    @Override
    public void onPostCreate(Bundle savedInstanceState) {
        super.onPostCreate(savedInstanceState);
        // The AutoCompleteTextView preserves its contents from previous instances, so if a URL was
        // set previously we want to apply it again. The text is restored in onRestoreInstanceState,
        // which occurs after onCreate and onStart, but before onPostCreate, so we get the URL here.
        String sceneUrl = sceneSelector.getCurrentString();

        map = view.getMap(this);
        map.loadSceneFile(sceneUrl, sceneUpdates);

        map.setZoom(16);
        map.setPosition(new LngLat(-74.00976419448854, 40.70532700869127));
        map.setHttpHandler(getHttpHandler());
        map.setTapResponder(this);
        map.setDoubleTapResponder(this);
        map.setLongPressResponder(this);
        map.setFeaturePickListener(this);
        map.setLabelPickListener(this);
        map.setMarkerPickListener(this);

        map.setViewCompleteListener(new ViewCompleteListener() {
            public void onViewComplete() {
                Log.d(TAG, "View complete");
            }});

        markers = map.addDataLayer("touch");
    }

    @Override
    public void onResume() {
        super.onResume();
        view.onResume();
    }

    @Override
    public void onPause() {
        super.onPause();
        view.onPause();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        view.onDestroy();
    }

    @Override
    public void onLowMemory() {
        super.onLowMemory();
        view.onLowMemory();
    }


    @Override
    public void onSceneReady(int sceneId, SceneError sceneError) {

        Log.d(TAG, "onSceneReady!");
        if (sceneError == null) {
            Toast.makeText(this, "Scene ready: " + sceneId, Toast.LENGTH_SHORT).show();
        } else {
            Toast.makeText(this, "Scene load error: " + sceneId + " "
                    + sceneError.getSceneUpdate().toString()
                    + " " + sceneError.getError().toString(), Toast.LENGTH_SHORT).show();

            Log.d(TAG, "Scene update errors "
                    + sceneError.getSceneUpdate().toString()
                    + " " + sceneError.getError().toString());
        }
    }

    HttpHandler getHttpHandler() {
        File cacheDir = getExternalCacheDir();
        if (cacheDir != null && cacheDir.exists()) {
            CachePolicy cachePolicy = new CachePolicy() {
                CacheControl tileCacheControl = new CacheControl.Builder().maxStale(7, TimeUnit.DAYS).build();
                @Override
                public CacheControl apply(HttpUrl url) {
                    if (url.host().equals("tile.mapzen.com")) {
                        return tileCacheControl;
                    }
                    return null;
                }
            };
            return new HttpHandler(new File(cacheDir, "tile_cache"), 30 * 1024 * 1024, cachePolicy);
        }
        return new HttpHandler();
    }

    @Override
    public boolean onSingleTapUp(float x, float y) {
        return false;
    }

    @Override
    public boolean onSingleTapConfirmed(float x, float y) {
        LngLat tappedPoint = map.screenPositionToLngLat(new PointF(x, y));

        if (lastTappedPoint != null) {
            Map<String, String> props = new HashMap<>();
            props.put("type", "line");
            props.put("color", "#D2655F");

            List<LngLat> line = new ArrayList<>();
            line.add(lastTappedPoint);
            line.add(tappedPoint);
            markers.addPolyline(line, props);

            Marker p = map.addMarker();
            p.setStylingFromPath(pointStylingPath);
            p.setPoint(tappedPoint);
            pointMarkers.add(p);
        }

        lastTappedPoint = tappedPoint;

        map.pickFeature(x, y);
        map.pickLabel(x, y);
        map.pickMarker(x, y);

        map.setPositionEased(tappedPoint, 1000);

        return true;
    }

    @Override
    public boolean onDoubleTap(float x, float y) {
        map.setZoomEased(map.getZoom() + 1.f, 500);
        LngLat tapped = map.screenPositionToLngLat(new PointF(x, y));
        LngLat current = map.getPosition();
        LngLat next = new LngLat(
                .5 * (tapped.longitude + current.longitude),
                .5 * (tapped.latitude + current.latitude));
        map.setPositionEased(next, 500);
        return true;
    }

    @Override
    public void onLongPress(float x, float y) {
        map.removeAllMarkers();
        pointMarkers.clear();
        markers.clear();
        showTileInfo = !showTileInfo;
        map.setDebugFlag(MapController.DebugFlag.TILE_INFOS, showTileInfo);
    }

    @Override
    public void onFeaturePick(Map<String, String> properties, float positionX, float positionY) {
        if (properties.isEmpty()) {
            Log.d(TAG, "Empty selection");
            return;
        }

        String name = properties.get("name");
        if (name.isEmpty()) {
            name = "unnamed";
        }

        Log.d(TAG, "Picked: " + name);
        final String message = name;
        Toast.makeText(getApplicationContext(), "Selected: " + message, Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onLabelPick(LabelPickResult labelPickResult, float positionX, float positionY) {
        if (labelPickResult == null) {
            Log.d(TAG, "Empty label selection");
            return;
        }

        String name = labelPickResult.getProperties().get("name");
        if (name.isEmpty()) {
            name = "unnamed";
        }

        Log.d(TAG, "Picked label: " + name);
        final String message = name;
        Toast.makeText(getApplicationContext(), "Selected label: " + message, Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onMarkerPick(MarkerPickResult markerPickResult, float positionX, float positionY) {
        if (markerPickResult == null) {
            Log.d(TAG, "Empty marker selection");
            return;
        }

        Log.d(TAG, "Picked marker: " + markerPickResult.getMarker().getMarkerId());
        final String message = String.valueOf(markerPickResult.getMarker().getMarkerId());
        Toast.makeText(getApplicationContext(), "Selected Marker: " + message, Toast.LENGTH_SHORT).show();
    }
}

