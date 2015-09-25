package com.mapzen.tangram;

import com.squareup.okhttp.Cache;
import com.squareup.okhttp.Callback;
import com.squareup.okhttp.OkHttpClient;
import com.squareup.okhttp.Request;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.TimeUnit;

public class HttpHandler {

    private OkHttpClient okClient;
    private Request.Builder okRequestBuilder;

    public HttpHandler() {
        okRequestBuilder = new Request.Builder();
        okClient = new OkHttpClient();
        okClient.setConnectTimeout(10, TimeUnit.SECONDS);
        okClient.setReadTimeout(30, TimeUnit.SECONDS);
    }

    public boolean onRequest(String url, Callback cb) {
        Request request = okRequestBuilder.tag(url).url(url).build();
        okClient.newCall(request).enqueue(cb);
        return true;
    }

    public void onCancel(String url) {
        okClient.cancel(url);
    }

    /**
     * Cache map data in a directory with a specified size limit
     * @param directory Directory in which map data will be cached
     * @param maxSize Maximum size of data to cache, in bytes
     * @return true if cache was successfully created
     */
    public boolean setCache(File directory, long maxSize) {
        try {
            Cache okTileCache = new Cache(directory, maxSize);
            okClient.setCache(okTileCache);
        } catch (IOException ignored) { return false; }
        return true;
    }

}
