package com.mapzen.tangram.viewholder;

import android.content.Context;
import android.util.Log;
import android.view.TextureView;

import androidx.annotation.NonNull;

import com.mapzen.tangram.BuildConfig;

public class TextureViewHolderFactory implements GLViewHolderFactory {

    private final boolean translucent;

    /**
     * Create a {@link GLViewHolderFactory} that builds a {@link TextureViewHolder}.
     * @param translucent Enables translucency in the {@link TextureView}
     */
    public TextureViewHolderFactory(boolean translucent) {
        this.translucent = translucent;
    }

    /**
     * Create a {@link GLViewHolderFactory} that builds a {@link TextureViewHolder}.
     */
    public TextureViewHolderFactory() {
        this(false);
    }

    /**
     * Responsible to create an instance of {@link TextureViewHolder} which holds
     * an instance of {@link TextureView} for map display.
     * @param context Application Context
     * @return {@link TextureViewHolder}
     */
    @Override
    @NonNull
    public GLViewHolder build(Context context) {
        TextureView textureView = new TextureView(context);
        TextureViewHolder textureViewHolder = new TextureViewHolder(textureView);

        textureViewHolder.setEGLContextClientVersion(2);
        textureViewHolder.setPreserveEGLContextOnPause(true);

        int alphaBits = translucent ? 8 : 0;
        textureViewHolder.setEGLConfigChooser(new ConfigChooser(8, 8, 8, alphaBits, 16, 8));
        textureView.setOpaque(!translucent);

        return textureViewHolder;
    }
}
