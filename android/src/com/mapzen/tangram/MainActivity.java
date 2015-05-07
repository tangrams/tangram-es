package com.mapzen.tangram;

import android.app.Activity;
import android.os.Bundle;
import android.view.Window;

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

