package com.mapzen.tangram.networking;

import android.os.Build;
import android.support.annotation.NonNull;
import android.util.Log;

import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

import javax.net.SocketFactory;
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


        builder.socketFactory(new SocketFactory() {
            SocketFactory factory = SocketFactory.getDefault();

            @Override
            public Socket createSocket() throws IOException {
                Socket s = factory.createSocket();
                try {
                    //Log.d("Tangram", "Patching socket nodelay: " + s.getTcpNoDelay());
                    s.setTcpNoDelay(true);
                } catch (SocketException e) {
                    // empty
                }
                return s;
            }

            @Override
            public Socket createSocket(String s, int i) throws IOException, UnknownHostException {
                return null;
            }

            @Override
            public Socket createSocket(String s, int i, InetAddress inetAddress, int i1) throws IOException, UnknownHostException {
                return null;
            }

            @Override
            public Socket createSocket(InetAddress inetAddress, int i) throws IOException {
                return null;
            }

            @Override
            public Socket createSocket(InetAddress inetAddress, int i, InetAddress inetAddress1, int i1) throws IOException {
                return null;
            }
        });

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
        final Request request = builder.build();
        Call call = okClient.newCall(request);
        call.enqueue(callback);
        return call;
    }

    @Override
    public void cancelRequest(final Object request) {
        if (request instanceof Call) {
            Call call = (Call)request;
            call.cancel();
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
