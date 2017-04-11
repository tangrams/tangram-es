package com.mapzen.tangram;

import okhttp3.Cache;
import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.OkHttpClient;
import okhttp3.Request;

import java.io.File;
import java.util.concurrent.TimeUnit;

/**
 * FIXME: Rename to UrlHandler.
 * {@code HttpHandler} is a class for customizing HTTP requests for map resources, it can be
 * extended to override the request or caching behavior.
 */
public class HttpHandler {

    private OkHttpClient okClient;

    /**
     * Construct an {@code HttpHandler} with default options.
     */
    public HttpHandler() {
        this(null, 0);
    }

    /**
     * Construct an {@code HttpHandler} with cache.
     * Cache map data in a directory with a specified size limit
     * @param directory Directory in which map data will be cached
     * @param maxSize Maximum size of data to cache, in bytes
     */
    public HttpHandler(File directory, long maxSize) {
        OkHttpClient.Builder builder = new OkHttpClient.Builder()
                .followRedirects(true)
                .followSslRedirects(true)
                .connectTimeout(10, TimeUnit.SECONDS)
                .writeTimeout(10, TimeUnit.SECONDS)
                .readTimeout(30, TimeUnit.SECONDS);

        if (directory != null && maxSize > 0) {
            builder.cache(new Cache(directory, maxSize));
        }

        okClient = builder.build();
    }

    /**
     * Begin an HTTP request
     * @param url URL for the requested resource
     * @param cb Callback for handling request result
     * @param requestHandle the identifier for the request
     * @return The Okhttp3.Call enqueued for execution
     */
    public void onRequest(String url, Callback cb, long requestHandle) {
        Request request = new Request.Builder()
                .url(url)
                .tag(requestHandle)
                .build();
        Call call = okClient.newCall(request);
        call.enqueue(cb);
    }

   /**
    * Cancel an HTTP request
    * @param requestHandle the identifier for the request to be cancelled
    */
   public void onCancel(long requestHandle) {
       // check and cancel running call
       for (Call runningCall : okClient.dispatcher().runningCalls()) {
           if (runningCall.request().tag().equals(requestHandle)) {
               runningCall.cancel();
           }
       }

       // check and cancel queued call
       for (Call queuedCall : okClient.dispatcher().queuedCalls()) {
           if (queuedCall.request().tag().equals(requestHandle)) {
               queuedCall.cancel();
           }
       }
   }

}
