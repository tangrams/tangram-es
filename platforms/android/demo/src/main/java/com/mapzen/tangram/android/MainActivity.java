package com.mapzen.tangram.android;

import android.content.Context;
import android.graphics.PointF;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Filter;
import android.widget.Spinner;
import android.widget.Toast;
import android.widget.AutoCompleteTextView;

import com.mapzen.tangram.HttpHandler;
import com.mapzen.tangram.LngLat;
import com.mapzen.tangram.MapController;
import com.mapzen.tangram.MapController.FeaturePickListener;
import com.mapzen.tangram.MapController.LabelPickListener;
import com.mapzen.tangram.MapController.MarkerPickListener;
import com.mapzen.tangram.MapController.ViewCompleteListener;
import com.mapzen.tangram.MapController.SceneUpdateErrorListener;
import com.mapzen.tangram.MapData;
import com.mapzen.tangram.Marker;
import com.mapzen.tangram.SceneUpdate;
import com.mapzen.tangram.SceneUpdateError;
import com.mapzen.tangram.MapView;
import com.mapzen.tangram.MapView.OnMapReadyCallback;
import com.mapzen.tangram.Marker;
import com.mapzen.tangram.MarkerPickResult;
import com.mapzen.tangram.SceneUpdate;
import com.mapzen.tangram.TouchInput.DoubleTapResponder;
import com.mapzen.tangram.TouchInput.LongPressResponder;
import com.mapzen.tangram.TouchInput.TapResponder;
import com.mapzen.tangram.LabelPickResult;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MainActivity extends AppCompatActivity implements OnMapReadyCallback, TapResponder,
        DoubleTapResponder, LongPressResponder, FeaturePickListener, LabelPickListener, MarkerPickListener, SceneUpdateErrorListener,
        AdapterView.OnItemSelectedListener, AdapterView.OnItemClickListener {

    private final String apiKey = "vector-tiles-tyHL4AY";
    private ArrayList<SceneUpdate> sceneUpdates = new ArrayList<>();

    /* Override ArrayAdaptor to include any text */
    private class CustomArrayAdapter extends ArrayAdapter {

        List<String> suggestions;

        Filter nameFilter = new Filter() {
            @Override
            public CharSequence convertResultToString(Object resultValue) {
                return resultValue.toString().toLowerCase();
            }

            @Override
            protected FilterResults performFiltering(CharSequence constraint) {
                if (constraint != null) {
                    suggestions.clear();
                    for (String scene: scenes) {
                        if (scene.toLowerCase().contains(constraint.toString().toLowerCase())) {
                            suggestions.add(scene);
                        }
                    }
                    if (suggestions.isEmpty()) {
                        suggestions.add(constraint.toString().toLowerCase()); //include the text when nothing matches
                    }

                    FilterResults filterResults = new FilterResults();
                    filterResults.values = suggestions;
                    filterResults.count = suggestions.size();
                    return filterResults;
                } else {
                    return new FilterResults();
                }
            }

            @Override
            protected void publishResults(CharSequence constraint, FilterResults results) {
                List<String> filterList = (ArrayList<String>) results.values;
                if (results != null && results.count > 0) {
                    clear();
                    for (String scene: filterList) {
                        add(scene);
                        notifyDataSetChanged();
                    }
                }
            }
        };

        CustomArrayAdapter(Context context, int resource, List<String> objects) {
            super(context, resource, objects);
            suggestions = new ArrayList<>();
        }

        @Override public Filter getFilter() {
            return nameFilter;
        }
    }

    MapController map;
    MapView view;
    LngLat lastTappedPoint;
    MapData markers;

    private final String[] scenes = {
        "scene.yaml",
        "https://mapzen.com/carto/bubble-wrap-style/bubble-wrap-style.zip",
        "https://mapzen.com/carto/refill-style/refill-style.zip",
        "https://mapzen.com/carto/walkabout-style/walkabout-style.zip",
        "https://mapzen.com/carto/tron-style/tron-style.zip",
        "https://mapzen.com/carto/cinnabar-style/cinnabar-style.zip",
        "https://mapzen.com/carto/zinc-style/zinc-style.zip"
    };

    AutoCompleteTextView autoCompleteView;
    CustomArrayAdapter customAdapter;
    Spinner spinner;

    String pointStylingPath = "layers.touch.point.draw.icons";
    ArrayList<Marker> pointMarkers = new ArrayList<Marker>();

    boolean showTileInfo = false;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        sceneUpdates.add(new SceneUpdate("global.sdk_mapzen_api_key", apiKey));
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);

        setContentView(R.layout.main);

        /* setup autocompletetextview adapter */
        autoCompleteView = (AutoCompleteTextView)findViewById(R.id.autoCompleteTextView);
        List<String> lst = new ArrayList<String>(Arrays.asList(scenes));
        customAdapter = new CustomArrayAdapter(this, android.R.layout.simple_list_item_1, lst);
        autoCompleteView.setAdapter(customAdapter);
        autoCompleteView.setThreshold(1);
        autoCompleteView.setOnItemClickListener(this);

        /* setup spinner style selecter */
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayShowTitleEnabled(false);
        spinner = (Spinner) findViewById(R.id.spinner);
        ArrayAdapter<CharSequence> sceneSelector =
                ArrayAdapter.createFromResource(this, R.array.style_array, R.layout.simple_spinner_item);
        sceneSelector.setDropDownViewResource(R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(sceneSelector);
        spinner.setOnItemSelectedListener(this);

        /* setup map view */
        view = (MapView)findViewById(R.id.map);
        view.onCreate(savedInstanceState);
        autoCompleteView.setText(scenes[0]);
        view.getMapAsync(this, "scene.yaml", sceneUpdates);
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

    @Override public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        int selectedIndex = 7; // custom
        String sceneText = autoCompleteView.getText().toString();
        for (int i = 0; i < scenes.length; i++) {
            if (sceneText.equals(scenes[i])) {
                selectedIndex = i;
                break;
            }
        }
        spinner.setSelection(selectedIndex);

        autoCompleteView.clearFocus();
        InputMethodManager in = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        in.hideSoftInputFromWindow(autoCompleteView.getWindowToken(), 0);

        map.loadSceneFile(autoCompleteView.getText().toString(),sceneUpdates);
    }

    @Override public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        if (map == null) { return; }

        if (position < scenes.length) {
            autoCompleteView.setText(scenes[position]);
            map.loadSceneFile(scenes[position], sceneUpdates);
        } else {
            autoCompleteView.setText(null);
            autoCompleteView.requestFocus();
            InputMethodManager in = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
            in.showSoftInput(autoCompleteView, 0);
        }
    }

    @Override public void onNothingSelected(AdapterView<?> parent) {
        // Do nothing.
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
                    runOnUiThread(new Runnable() {
                            public void run() {
                                Log.d("Tangram", "View complete");
                            }
                        });
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
        runOnUiThread(new Runnable() {
                          @Override
                          public void run() {
                              Toast.makeText(getApplicationContext(),
                                      "Selected: " + message,
                                      Toast.LENGTH_SHORT).show();
                          }
                      });

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
        runOnUiThread(new Runnable() {
                          @Override
                          public void run() {
                              Toast.makeText(getApplicationContext(),
                                      "Selected label: " + message,
                                      Toast.LENGTH_SHORT).show();
                          }
                      });
    }

    @Override
    public void onMarkerPick(MarkerPickResult markerPickResult, float positionX, float positionY) {
        if (markerPickResult == null) {
            Log.d("Tangram", "Empty marker selection");
            return;
        }

        Log.d("Tangram", "Picked marker: " + markerPickResult.getMarker().getMarkerId());
        final String message = String.valueOf(markerPickResult.getMarker().getMarkerId());
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(getApplicationContext(),
                        "Selected Marker: " + message,
                        Toast.LENGTH_SHORT).show();
            }
        });
    }

    @Override
    public void onSceneUpdateError(SceneUpdateError sceneUpdateError) {
        Log.d("Tangram", "Scene update errors "
                + sceneUpdateError.getSceneUpdate().toString()
                + " " + sceneUpdateError.getError().toString());
    }
}

