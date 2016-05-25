package com.mapzen.tangram;

import com.squareup.okhttp.Cache;
import com.squareup.okhttp.Callback;
import com.squareup.okhttp.OkHttpClient;
import com.squareup.okhttp.Request;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.TimeUnit;

/**
 * {@code HttpHandler} is a class for customizing HTTP requests for map resources, it can be
 * extended to override the request or caching behavior.
 */
public class HttpHandler {

    private OkHttpClient okClient;
    protected Request.Builder okRequestBuilder;

    /**
     * Construct an {@code HttpHandler} with default options.
     */
    public HttpHandler() {
        okRequestBuilder = new Request.Builder();
        okClient = new OkHttpClient();
        okClient.setConnectTimeout(10, TimeUnit.SECONDS);
        okClient.setReadTimeout(30, TimeUnit.SECONDS);
    }

    /**
     * Begin an HTTP request
     * @param url URL for the requested resource
     * @param cb Callback for handling request result
     * @return true if request was successfully started
     */
    public boolean onRequest(String url, Callback cb) {
        Request request = okRequestBuilder.tag(url).url(url).build();
        okClient.newCall(request).enqueue(cb);
        return true;
    }

    /**
     * Cancel an HTTP request
     * @param url URL of the request to be cancelled
     */
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
        Cache okTileCache = new Cache(directory, maxSize);
        okClient.setCache(okTileCache);

        return true;
    }

}
