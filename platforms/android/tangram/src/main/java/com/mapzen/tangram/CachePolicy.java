package com.mapzen.tangram;

import okhttp3.CacheControl;
import okhttp3.HttpUrl;

/**
 * A CachePolicy evaluates a {@link HttpUrl} and determines the {@link CacheControl} that should
 * apply to a request for that URL.
 */
public interface CachePolicy {
    /**
     * Apply a caching policy to a URL.
     * @param url The URL being requested
     * @return The CacheControl to apply to the request, or {@code null} for the default behavior.
     */
    CacheControl apply(final HttpUrl url);
}
