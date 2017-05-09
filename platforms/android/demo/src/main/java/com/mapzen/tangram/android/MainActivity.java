package com.mapzen.tangram.android;

import android.graphics.PointF;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Window;
import android.widget.Toast;

import com.mapzen.tangram.HttpHandler;
import com.mapzen.tangram.LabelPickResult;
import com.mapzen.tangram.LngLat;
import com.mapzen.tangram.MapController;
import com.mapzen.tangram.MapController.FeaturePickListener;
import com.mapzen.tangram.MapController.LabelPickListener;
import com.mapzen.tangram.MapController.MarkerPickListener;
import com.mapzen.tangram.MapController.SceneUpdateErrorListener;
import com.mapzen.tangram.MapController.ViewCompleteListener;
import com.mapzen.tangram.MapData;
import com.mapzen.tangram.MapView;
import com.mapzen.tangram.MapView.OnMapReadyCallback;
import com.mapzen.tangram.Marker;
import com.mapzen.tangram.MarkerPickResult;
import com.mapzen.tangram.SceneUpdate;
import com.mapzen.tangram.SceneUpdateError;
import com.mapzen.tangram.TouchInput.DoubleTapResponder;
import com.mapzen.tangram.TouchInput.LongPressResponder;
import com.mapzen.tangram.TouchInput.TapResponder;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MainActivity extends AppCompatActivity implements OnMapReadyCallback, TapResponder,
        DoubleTapResponder, LongPressResponder, FeaturePickListener, LabelPickListener, MarkerPickListener, SceneUpdateErrorListener {

    private static final String MAPZEN_API_KEY = BuildConfig.MAPZEN_API_KEY;

    private static final String[] SCENE_PRESETS = {
            "asset:///scene.yaml",
            "https://mapzen.com/carto/bubble-wrap-style-more-labels/bubble-wrap-style-more-labels.zip",
            "https://mapzen.com/carto/refill-style-more-labels/refill-style-more-labels.zip",
            "https://mapzen.com/carto/walkabout-style-more-labels/walkabout-style-more-labels.zip",
            "https://mapzen.com/carto/tron-style-more-labels/tron-style-more-labels.zip",
            "https://mapzen.com/carto/cinnabar-style-more-labels/cinnabar-style-more-labels.zip",
            "https://mapzen.com/carto/zinc-style-more-labels/zinc-style-more-labels.zip"
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

        // Create a scene update to apply our API key in the scene.
        sceneUpdates.add(new SceneUpdate("global.sdk_mapzen_api_key", MAPZEN_API_KEY));

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
        view.getMapAsync(this, sceneUrl, sceneUpdates);
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
    public void onMapReady(MapController mapController) {
        map = mapController;
        map.setZoom(16);
        map.setPosition(new LngLat(-74.00976419448854, 40.70532700869127));
        map.setHttpHandler(getHttpHandler());
        map.setTapResponder(this);
        map.setDoubleTapResponder(this);
        map.setLongPressResponder(this);
        map.setFeaturePickListener(this);
        map.setLabelPickListener(this);
        map.setMarkerPickListener(this);
        map.setSceneUpdateErrorListener(this);

        map.setViewCompleteListener(new ViewCompleteListener() {
                public void onViewComplete() {
                    Log.d("Tangram", "View complete");
                }});
        markers = map.addDataLayer("touch");
    }

    HttpHandler getHttpHandler() {
        File cacheDir = getExternalCacheDir();
        if (cacheDir != null && cacheDir.exists()) {
            return new HttpHandler(new File(cacheDir, "tile_cache"), 30 * 1024 * 1024);
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
            Log.d("Tangram", "Empty selection");
            return;
        }

        String name = properties.get("name");
        if (name.isEmpty()) {
            name = "unnamed";
        }

        Log.d("Tangram", "Picked: " + name);
        final String message = name;
        Toast.makeText(getApplicationContext(), "Selected: " + message, Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onLabelPick(LabelPickResult labelPickResult, float positionX, float positionY) {
        if (labelPickResult == null) {
            Log.d("Tangram", "Empty label selection");
            return;
        }

        String name = labelPickResult.getProperties().get("name");
        if (name.isEmpty()) {
            name = "unnamed";
        }

        Log.d("Tangram", "Picked label: " + name);
        final String message = name;
        Toast.makeText(getApplicationContext(), "Selected label: " + message, Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onMarkerPick(MarkerPickResult markerPickResult, float positionX, float positionY) {
        if (markerPickResult == null) {
            Log.d("Tangram", "Empty marker selection");
            return;
        }

        Log.d("Tangram", "Picked marker: " + markerPickResult.getMarker().getMarkerId());
        final String message = String.valueOf(markerPickResult.getMarker().getMarkerId());
        Toast.makeText(getApplicationContext(), "Selected Marker: " + message, Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onSceneUpdateError(SceneUpdateError sceneUpdateError) {
        Log.d("Tangram", "Scene update errors "
                + sceneUpdateError.getSceneUpdate().toString()
                + " " + sceneUpdateError.getError().toString());
    }
}

