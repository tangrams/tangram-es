package com.mapzen.tangram.viewholder;

import android.content.Context;

public interface GLViewHolderFactory {
    /**
     * @param context Application Context
     */
    GLViewHolder build(Context context);
}
