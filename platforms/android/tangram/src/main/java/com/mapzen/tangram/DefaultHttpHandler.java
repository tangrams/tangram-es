package com.mapzen.tangram;

import android.os.Build;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.io.File;
import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.TimeUnit;

import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;

import okhttp3.Cache;
import okhttp3.CacheControl;
import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.ConnectionSpec;
import okhttp3.HttpUrl;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;
import okhttp3.ResponseBody;
import okhttp3.TlsVersion;

/**
 * {@code DefaultHttpHandler} is a provides a default implementation for {@link HttpHandler}
 * for customizing HTTP requests for map resources.
 */
public class DefaultHttpHandler implements HttpHandler {

    protected OkHttpClient okClient;
    protected CachePolicy cachePolicy;
    protected CacheControl tileCacheControl;

    /**
     * Enables TLS v1.2 when creating SSLSockets.
     * <p>
     * For some reason, android supports TLS v1.2 from API 16, but enables it by
     * default only from API 20.
     *
     * @link https://developer.android.com/reference/javax/net/ssl/SSLSocket.html
     * @see SSLSocketFactory
     */
    private class Tls12SocketFactory extends SSLSocketFactory {
        private final String[] TLS_V12_ONLY = {"TLSv1.2"};

        final SSLSocketFactory delegate;

        public Tls12SocketFactory(final SSLSocketFactory base) {
            this.delegate = base;
        }

        @Override
        public String[] getDefaultCipherSuites() {
            return delegate.getDefaultCipherSuites();
        }

        @Override
        public String[] getSupportedCipherSuites() {
            return delegate.getSupportedCipherSuites();
        }

        @Override
        public Socket createSocket(final Socket s, final String host, final int port, final boolean autoClose) throws IOException {
            return patch(delegate.createSocket(s, host, port, autoClose));
        }

        @Override
        public Socket createSocket(final String host, final int port) throws IOException, UnknownHostException {
            return patch(delegate.createSocket(host, port));
        }

        @Override
        public Socket createSocket(final String host, final int port, final InetAddress localHost,
                                   final int localPort) throws IOException, UnknownHostException {
            return patch(delegate.createSocket(host, port, localHost, localPort));
        }

        @Override
        public Socket createSocket(InetAddress host, int port) throws IOException {
            return patch(delegate.createSocket(host, port));
        }

        @Override
        public Socket createSocket(final InetAddress address, final int port, final InetAddress localAddress,
                                   final int localPort) throws IOException {
            return patch(delegate.createSocket(address, port, localAddress, localPort));
        }

        private Socket patch(final Socket s) {
            if (s instanceof SSLSocket) {
                ((SSLSocket) s).setEnabledProtocols(TLS_V12_ONLY);
            }
            return s;
        }
    }

    /**
     * Construct an {@code DefaultHttpHandler} with default options.
     */
    public DefaultHttpHandler() {
        this(null, 0, null, null);
    }

    /**
     * Construct an {@code DefaultHttpHandler} with cache.
     * Cache map data in a directory with a specified size limit
     * @param directory Directory in which map data will be cached
     * @param maxSize Maximum size of data to cache, in bytes
     */
    public DefaultHttpHandler(@Nullable final File directory, final long maxSize) {
        this(directory, maxSize, null, null);
    }

    /**
     * Construct an {@code DefaultHttpHandler} with cache.
     * Cache map data in a directory with a specified size limit
     * @param directory Directory in which map data will be cached
     * @param maxSize Maximum size of data to cache, in bytes
     * @param policy Cache policy to apply on requests
     * @param cacheControl {@link CacheControl} used to provide caching based on {@link CachePolicy}
     */
    public DefaultHttpHandler(@Nullable final File directory, final long maxSize, @Nullable final CachePolicy policy, @Nullable final CacheControl cacheControl) {
        final OkHttpClient.Builder builder = new OkHttpClient.Builder()
                .followRedirects(true)
                .followSslRedirects(true)
                .connectTimeout(10, TimeUnit.SECONDS)
                .writeTimeout(10, TimeUnit.SECONDS)
                .readTimeout(30, TimeUnit.SECONDS);

        if (directory != null && maxSize > 0) {
            builder.cache(new Cache(directory, maxSize));
        }

        // Use specified policy or construct default if null.
        cachePolicy = policy;
        tileCacheControl = cacheControl;
        if (cachePolicy == null || tileCacheControl == null) {
            cachePolicy = new CachePolicy() {
                @Override
                public boolean apply(@NonNull final String url) {
                    return false;
                }
            };
        }

        if (Build.VERSION.SDK_INT < 22) {
            try {
                final SSLContext sc = SSLContext.getInstance("TLSv1.2");
                sc.init(null, null, null);
                builder.sslSocketFactory(new Tls12SocketFactory(sc.getSocketFactory()));

                final ConnectionSpec cs = new ConnectionSpec.Builder(ConnectionSpec.MODERN_TLS)
                        .tlsVersions(TlsVersion.TLS_1_2)
                        .build();

                final List<ConnectionSpec> specs = new ArrayList<>();
                specs.add(cs);
                specs.add(ConnectionSpec.COMPATIBLE_TLS);
                specs.add(ConnectionSpec.CLEARTEXT);

                builder.connectionSpecs(specs);
            } catch (final Exception exc) {
                android.util.Log.e("Tangram", "Error while setting TLS 1.2", exc);
            }
        }

        okClient = builder.build();
    }

    @Override
    public Object startRequest(@NonNull final String url, @NonNull final HttpHandler.Callback cb) {
        final HttpUrl httpUrl = HttpUrl.parse(url);
        Call call = null;
        if (httpUrl == null) {
            cb.onFailure(new IOException("HttpUrl failed to parse url=" + url));
        }
        else {
            // Construct okhttp3.Callback which forwards apt response calls to internal HttpResponse.Callback
            final okhttp3.Callback callback = new okhttp3.Callback() {
                @Override
                public void onFailure(final Call call, final IOException e) {
                    if (call.isCanceled()) {
                        cb.onCancel();
                        return;
                    }
                    cb.onFailure(e);
                }

                @Override
                public void onResponse(final Call call, final Response response) throws IOException {
                    if (!response.isSuccessful()) {
                        IOException e = new IOException("Unexpected response code: " + response + " for URL: " + url);
                        cb.onFailure(e);
                        throw e;
                    }
                    final ResponseBody body = response.body();
                    if (body == null) {
                        IOException e = new IOException("Unexpected null body for URL: " + url);
                        cb.onFailure(e);
                        throw e;
                    }
                    else {
                        // TODO headers? cacheControl?
                        cb.onResponse(response.code(), body.bytes(), new HashMap<String, String>());
                    }
                }
            };
            final Request.Builder builder = new Request.Builder().url(httpUrl);
            // TODO: cachePolicy using headers??
            if (cachePolicy.apply(url)) {
                builder.cacheControl(tileCacheControl);
            }
            final Request request = builder.build();
            call = okClient.newCall(request);
            call.enqueue(callback);
        }
        return call;
    }

    @Override
    public void cancelRequest(final Object cancelObj) {
        Call call = (Call)cancelObj;
        call.cancel();
    }

}
