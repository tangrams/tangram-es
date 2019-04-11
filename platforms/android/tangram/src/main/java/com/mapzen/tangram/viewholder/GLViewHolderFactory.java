package com.mapzen.tangram.viewholder;

import android.content.Context;
import android.support.annotation.NonNull;

public interface GLViewHolderFactory {
    /**
     * @param context Application Context
     */
    GLViewHolder build(Context context);
}
