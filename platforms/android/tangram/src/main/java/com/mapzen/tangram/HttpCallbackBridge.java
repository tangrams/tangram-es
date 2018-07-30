package com.mapzen.tangram;

import java.io.IOException;

/**
 * {@code HttpCallbackBridge} interface implemented internally in {@link MapController#startUrlRequest(String, long)}
 * Passes network responses from underlying implementation to be processed internally
 */
public interface HttpCallbackBridge {
    /**
     * Network request failed response
     * @param e Exception representing the failed network request
     */
    public void onFailure(IOException e);
    /**
     * Network request success response
     * @param rawDataBytes raw bytes from the body of a network response
     */
    public void onSuccess(byte[] rawDataBytes);
}
