package com.mapzen.tangram;

import android.support.annotation.NonNull;

/**
 * {@code HttpHandler} interface for handling network requests for map resources,
 * it can be implemented to provide non-default http network request or caching behavior.
 * To use client implemented HttpHandler use {@link MapController#setHttpHandler(HttpHandler)}
 */
public interface HttpHandler {
    /**
     * Begin an HTTP request
     * @param url URL for the requested resource
     * @param cb Callback for handling request result
     * @param requestHandle the identifier for the request
     */
    void onRequest(@NonNull final String url, @NonNull final HttpResponse cb, final long requestHandle);
    /**
     * Cancel an HTTP request
     * @param requestHandle the identifier for the request to be cancelled
     */
    void onCancel(final long requestHandle);
}

