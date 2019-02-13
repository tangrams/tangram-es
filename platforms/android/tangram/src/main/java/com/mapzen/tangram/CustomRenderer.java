package com.mapzen.tangram;

import android.support.annotation.Keep;

@Keep
public interface CustomRenderer {

    void initialize();

    void render(double width, double height, double longitude, double latitude, double zoom,
                double rotation, double tilt, double fieldOfView);

    void deinitialize();

}
