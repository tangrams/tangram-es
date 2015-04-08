package com.mapzen.tangram;

import java.nio.charset.Charset;

import com.squareup.okhttp.OkHttpClient;
import com.squareup.okhttp.Request;
import com.squareup.okhttp.RequestBody;
import com.squareup.okhttp.Response;
import com.squareup.okhttp.Call;

import okio.BufferedSource;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

public class OkHttpInterface
{
    private OkHttpClient client = new OkHttpClient();

    String rawDataOut;
    byte[] rawDataBytes;

    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("tangram");
    }

    public OkHttpInterface()
    {
        if(client == null) 
        {
            client = new OkHttpClient();
        }
        client.setConnectTimeout(10, TimeUnit.SECONDS);
        client.setReadTimeout(30, TimeUnit.SECONDS);
    }
    
    public boolean run(String url) throws Exception
    {
        Request request = new Request.Builder().url(url).build();

        try {
            Response response = client.newCall(request).execute();
            if (!response.isSuccessful()) throw new IOException("Unexpected code " + response);

            BufferedSource src = response.body().source();
            rawDataBytes = src.readByteArray();
        
            System.out.println("Tangram Java: " + url + " fetched bytes Final length: " + rawDataBytes.length);
            return true;
        } catch (Exception e) {
            System.out.println("Tangram Timeout??");
            return false;
        }
    }
}

