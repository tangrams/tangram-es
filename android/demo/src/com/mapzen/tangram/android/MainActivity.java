package com.mapzen.tangram.android;

import android.app.Activity;
import android.os.Bundle;
import android.view.Window;

import com.mapzen.tangram.MapController;
import com.mapzen.tangram.MapView;

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

    }

}

