package com.mapzen.tangram.android;

import android.app.Activity;
import android.os.Bundle;
import android.view.Window;
import com.mapzen.tangram.Tangram;

public class MainActivity extends Activity {

    Tangram tangram;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);

        tangram = new Tangram(this);

        setContentView(tangram.getView());
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        tangram.onDestroy();
    }

}

