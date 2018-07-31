package com.mapzen.tangram;

import android.support.annotation.NonNull;

/**
 * A CachePolicy evaluates a url (String) and determines the apt caching control that should be
 * applied to a request for that url.
 */
public interface CachePolicy {
    /**
     * Apply a caching policy to a URL.
     * @param url A string URL being requested
     * @return true to apply to apply client implemented caching control to the request, or false for the default behavior
     */
    boolean apply(@NonNull final String url);
}
