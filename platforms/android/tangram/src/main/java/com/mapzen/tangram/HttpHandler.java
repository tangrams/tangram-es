package com.mapzen.tangram;

import android.support.annotation.NonNull;

import java.io.IOException;
import java.util.Map;

/**
 * {@code HttpHandler} interface for handling network requests for map resources,
 * it can be implemented to provide non-default http network request or caching behavior.
 * To use client implemented HttpHandler provide one during map initialization {@link MapView#getMap(MapController.SceneLoadListener, HttpHandler)}
 */
public interface HttpHandler {
    /**
     * Begin an HTTP request
     * @param url URL for the requested resource
     * @param cb Callback for handling request result
     * @return identifier associated with this network request, used to canceling the request
     */
    Object startRequest(@NonNull final String url, @NonNull final Callback cb);
    /**
     * Cancel an HTTP request
     * @param object an identifier for the request to be cancelled
     */
    void cancelRequest(final Object object);

    /**
     * {@code Callback}
     * Passes network responses from underlying implementation to be processed internally
     */
    interface Callback {
        /**
         * Network request failed response
         * @param e Exception representing the failed network request
         */
        void onFailure(IOException e);
        /**
         * Network request success response
         * @param errorCode An integer error code returned from a network response
         * @param rawDataBytes raw bytes from the body of a network response
         * @param headers network package headers
         */
        void onResponse(final int errorCode, byte[] rawDataBytes, Map<String, String> headers);

        void onCancel();
    }
}

