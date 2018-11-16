package com.mapzen.tangram.networking;

import android.os.Build;
import android.support.annotation.NonNull;
import android.util.Log;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

import javax.net.ssl.SSLContext;

import okhttp3.Call;
import okhttp3.ConnectionSpec;
import okhttp3.HttpUrl;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;
import okhttp3.ResponseBody;
import okhttp3.TlsVersion;

/**
 * {@code DefaultHttpHandler} is an implementation of {@link HttpHandler} using OkHTTP.
 * Customize this class for your own application by subclassing and overriding
 * {@link DefaultHttpHandler#configureClient(OkHttpClient.Builder)} and
 * {@link DefaultHttpHandler#configureRequest(HttpUrl, Request.Builder)}.
 */
public class DefaultHttpHandler implements HttpHandler {

    private OkHttpClient okClient;

    /**
     * Construct an {@code DefaultHttpHandler} with default options.
     */
    public DefaultHttpHandler() {
        final OkHttpClient.Builder builder = new OkHttpClient.Builder()
                .followRedirects(true)
                .followSslRedirects(true)
                .connectTimeout(10, TimeUnit.SECONDS)
                .writeTimeout(10, TimeUnit.SECONDS)
                .readTimeout(30, TimeUnit.SECONDS);

        // For some reason, android supports TLS v1.2 from API 16, but enables it by default only from API 20.
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

        configureClient(builder);

        okClient = builder.build();
    }

    @Override
    public Object startRequest(@NonNull final String url, @NonNull final HttpHandler.Callback cb) {
        final HttpUrl httpUrl = HttpUrl.parse(url);
        if (httpUrl == null) {
            cb.onFailure(new IOException("Failed to parse URL: " + url));
            return null;
        }
        // Construct okhttp3.Callback which forwards response calls to HttpResponse.Callback
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
            public void onResponse(final Call call, final Response response) {
                byte[] data = null;
                final ResponseBody body = response.body();

                if (body != null) {
                    Log.e("Tangram", "body size:" + body.contentLength());
                    try {
                        data = body.bytes();
                        cb.onResponse(response.code(), data);
                    } catch (final IOException e) {
                        Log.e("Tangram", "Error reading bytes from response body:" + e.getMessage());
                        onFailure(call, e);
                    } finally {
                        response.close();
                    }
                }
            }
        };
        final Request.Builder builder = new Request.Builder().url(httpUrl);
        configureRequest(httpUrl, builder);
        //builder.addHeader("Accept-Encoding", "gzip");
        final Request request = builder.build();
        Call call = okClient.newCall(request);
        call.enqueue(callback);
        return call;
    }

    @Override
    public void cancelRequest(final Object request) {
        if (request instanceof Call) {

            Call call = (Call)request;
            if (okClient.dispatcher().runningCalls().indexOf(call) < 0) {
                call.cancel();
                Log.d("Tangram", "Cancel request" + call.toString());
            } else {
                Log.d("Tangram", "Skip Cancel" + call.toString());
            }
        }
    }

    /**
     * Override this method to customize the OkHTTP client
     * @param builder OkHTTP client builder to customize
     */
    protected void configureClient(OkHttpClient.Builder builder) {
    }

    /**
     * Override this method to customize HTTP requests
     * @param url Request URL
     * @param builder Request builder to customize
     */
    protected void configureRequest(HttpUrl url, Request.Builder builder) {
    }

}
