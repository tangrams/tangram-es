package com.mapzen.tangram.viewholder;

import android.content.Context;
import android.support.annotation.NonNull;

public interface GLViewHolderFactory {
    /**
     * Responsible to create an instance of {@link GLSurfaceViewHolder} or {@link TextureViewHolder} or
     * client provided implementation for {@link GLViewHolder}
     * @param context Application Context
     */
    GLViewHolder build(Context context);
}
