package com.mapzen.tangram;

import okhttp3.Cache;
import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.OkHttpClient;
import okhttp3.Request;

import java.io.File;
import java.util.concurrent.TimeUnit;

public class HttpHandler {

    private OkHttpClient okClient;
    private OkHttpClient.Builder okClientBuilder;
    private Request.Builder okRequestBuilder;

    public HttpHandler() {
        okRequestBuilder = new Request.Builder();
        okClientBuilder = new OkHttpClient().newBuilder()
                .connectTimeout(10, TimeUnit.SECONDS)
                .readTimeout(30, TimeUnit.SECONDS);
        okClient = okClientBuilder.build();
    }

    /**
     * Begin an HTTP request
     * @param url URL for the requested resource
     * @param cb Callback for handling request result
     * @return true if request was successfully started
     */
    public boolean onRequest(String url, Callback cb) {
        Request request = okRequestBuilder.url(url).build();
        okClient.newCall(request).enqueue(cb);
        return true;
    }

    /**
     * Cancel an HTTP request
     * @param url URL of the request to be cancelled
     */
    public void onCancel(String url) {
        for (Call call : okClient.dispatcher().queuedCalls()) {
            if (call.request().url().toString().equals(url)) {
                call.cancel();
            }
        }
        for (Call call : okClient.dispatcher().runningCalls()) {
            if (call.request().url().toString().equals(url)) {
                call.cancel();
            }
        }
    }

    /**
     * Cache map data in a directory with a specified size limit
     * @param directory Directory in which map data will be cached
     * @param maxSize Maximum size of data to cache, in bytes
     * @return true if cache was successfully created
     */
    public boolean setCache(File directory, long maxSize) {
        Cache okTileCache = new Cache(directory, maxSize);
        okClient = okClientBuilder.cache(okTileCache).build();
        return true;
    }

}
