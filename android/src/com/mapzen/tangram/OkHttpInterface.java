package com.mapzen.tangram;

import java.nio.charset.Charset;

import com.squareup.okhttp.OkHttpClient;
import com.squareup.okhttp.Request;
import com.squareup.okhttp.RequestBody;
import com.squareup.okhttp.Response;
import com.squareup.okhttp.Call;

import java.io.IOException;

public class OkHttpInterface
{
    private OkHttpClient client = new OkHttpClient();

    String rawDataOut;
    byte[] rawDataBytes;

    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("tangram");
    }
    
    public boolean run(String url) throws Exception
    {
        if(client == null) {
            client = new OkHttpClient();
        }

        Request request = new Request.Builder().url(url).build();
        Response response = client.newCall(request).execute();
        if (!response.isSuccessful()) throw new IOException("Unexpected code " + response);
        
        rawDataOut = response.body().string();
        rawDataBytes = rawDataOut.getBytes(Charset.forName("UTF-8"));
        System.out.println("Tangram Java: " + url + " fetched str Final length: " + rawDataOut.length());
        System.out.println("Tangram Java: " + url + " fetched bytes Final length: " + rawDataBytes.length);
        return true;
    }
}

