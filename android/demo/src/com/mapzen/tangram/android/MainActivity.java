package com.mapzen.tangram.android;

import android.app.Activity;
import android.os.Bundle;
import android.view.Window;
import android.view.View;
import com.mapzen.tangram.Tangram;

public class MainActivity extends Activity {

    Tangram tangram;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);

        setContentView(R.layout.main);

        tangram = (Tangram)findViewById(R.id.map);
        tangram.setup(this);


    }

}

