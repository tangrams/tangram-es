package com.almeros.android.multitouch;

import android.content.Context;
import android.view.MotionEvent;
import android.util.DisplayMetrics;

/**
 * @author Robert Nordan (robert.nordan@norkart.no)
 *
 * Copyright (c) 2013, Norkart AS
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
public class ShoveGestureDetector extends TwoFingerGestureDetector {

    /**
     * Listener which must be implemented which is used by ShoveGestureDetector
     * to perform callbacks to any implementing class which is registered to a
     * ShoveGestureDetector via the constructor.
     *
     * @see ShoveGestureDetector.SimpleOnShoveGestureListener
     */
    public interface OnShoveGestureListener {
        boolean onShove(ShoveGestureDetector detector);
        boolean onShoveBegin(ShoveGestureDetector detector);
        void onShoveEnd(ShoveGestureDetector detector);
    }

    /**
     * Helper class which may be extended and where the methods may be
     * implemented. This way it is not necessary to implement all methods
     * of OnShoveGestureListener.
     */
    public static class SimpleOnShoveGestureListener implements OnShoveGestureListener {
        public boolean onShove(ShoveGestureDetector detector) {
            return false;
        }

        public boolean onShoveBegin(ShoveGestureDetector detector) {
            return true;
        }

        public void onShoveEnd(ShoveGestureDetector detector) {
            // Do nothing, overridden implementation may be used
        }
    }

    private DisplayMetrics displayMetrics;

    private float mPrevFinger0Y;
    private float mCurrFinger0Y;
    private float mPrevFinger1Y;
    private float mCurrFinger1Y;

    private float mStartY0;
    private float mStartY1;

    private final OnShoveGestureListener mListener;
    private boolean mSloppyGesture;
    private boolean mInitiated;

    // Make sure there is sufficient drag in the 2 fingers (4.5% of minDim)
    private final float DRAG_THRESHOLD = 0.045f;
    // Make sure xSpan between 2 fingers is not increasing and is more or less consistent (1% of minDim)
    private final float XSPAN_THRESHOLD = 0.01f;

    public ShoveGestureDetector(Context context, OnShoveGestureListener listener) {
        super(context);
        displayMetrics = context.getResources().getDisplayMetrics();
        mListener = listener;
    }

    @Override
    protected void handleStartProgressEvent(int actionCode, MotionEvent event) {
        switch (actionCode) {
            case MotionEvent.ACTION_POINTER_DOWN:
                // At least the second finger is on screen now

                resetState(); // In case we missed an UP/CANCEL event
                mPrevEvent = MotionEvent.obtain(event);
                mTimeDelta = 0;

                mStartY0 = event.getY(0);
                mStartY1 = event.getY(1);

                updateStateByEvent(event);

                // Make this event explicitly sloppy
                mInitiated = isSloppyGesture(event);
                mSloppyGesture = true;
                break;

            case MotionEvent.ACTION_MOVE:

                // Check for previous MOVE events (stop doing when last MOVE was not sloppy and shove started)
                if (!mSloppyGesture && mInitiated) {
                    break;
                }

                // Update Previous
                if (mPrevEvent != null) {
                    mPrevEvent.recycle();
                }
                mPrevEvent = MotionEvent.obtain(event);

                if (mPrevEvent.getPointerCount() != 2) {
                    break;
                }

                updateStateByEvent(event);

                // See if we still have a sloppy gesture
                mSloppyGesture = isSloppyGesture(event);
                if (!mSloppyGesture) {
                    // No, start normal gesture now
                    mGestureInProgress = mListener.onShoveBegin(this);
                }

                break;

            case MotionEvent.ACTION_POINTER_UP:
                if (!mSloppyGesture) {
                    break;
                }

                break;
        }
    }


    @Override
    protected void handleInProgressEvent(int actionCode, MotionEvent event) {
        switch (actionCode) {
            case MotionEvent.ACTION_POINTER_UP:
                // Gesture ended but
                updateStateByEvent(event);

                if (!mSloppyGesture) {
                    mListener.onShoveEnd(this);
                }

                resetState();
                break;

            case MotionEvent.ACTION_CANCEL:
                if (!mSloppyGesture) {
                    mListener.onShoveEnd(this);
                }

                resetState();
                break;

            case MotionEvent.ACTION_MOVE:
                updateStateByEvent(event);

                // Only accept the event if our relative pressure is within
                // a certain limit. This can help filter shaky data as a
                // finger is lifted. Also check that shove is meaningful.
                if (mCurrPressure / mPrevPressure > PRESSURE_THRESHOLD
                        && Math.abs(getShovePixelsDelta()) > 0.5f) {
                    final boolean updatePrevious = mListener.onShove(this);
                    if (updatePrevious) {
                        mPrevEvent.recycle();
                        mPrevEvent = MotionEvent.obtain(event);
                    }
                }
                break;
        }
    }

    @Override
    protected void resetState() {
        super.resetState();
        mSloppyGesture = false;
        mInitiated = false;

        //mimic google map behavior (and use the first finger down for getting pixelDelta
        mCurrFinger0Y = 0.0f;
        mPrevFinger0Y = 0.0f;
        mCurrFinger1Y = 0.0f;
        mPrevFinger1Y = 0.0f;
    }

    @Override
    protected void updateStateByEvent(MotionEvent curr) {
        super.updateStateByEvent(curr);

        final MotionEvent prev = mPrevEvent;
        float py0 = prev.getY(0);
        float py1 = prev.getY(1);

        float cy0 = curr.getY(0);
        float cy1 = curr.getY(1);
        mCurrFinger0Y = cy0;
        mPrevFinger0Y = py0;
        mCurrFinger1Y = cy1;
        mPrevFinger1Y = py1;
    }

    @Override
    protected boolean isSloppyGesture(MotionEvent event) {
        boolean sloppy = super.isSloppyGesture(event);
        if (sloppy) {
            return true;
        }

        final float drag0 = event.getY(0) - mStartY0;
        final float drag1 = event.getY(1) - mStartY1;

        final float xSpanDiff = Math.abs(mCurrFingerDiffX - mPrevFingerDiffX);
        final float minDim = Math.min(displayMetrics.widthPixels, displayMetrics.heightPixels);
        final float minDrag = DRAG_THRESHOLD * minDim;

        if (drag0 * drag1 < 0.0f) { // Sloppy if fingers moving in opposite y direction
            return true;
        } else if (Math.abs(drag0) < minDrag || Math.abs(drag1) < minDrag) {
            return true;
        } else if (xSpanDiff > XSPAN_THRESHOLD * minDim) {
            return true;
        }

        // Do angle check post drag check!!
        double angle = Math.abs(Math.atan2(mCurrFingerDiffY, mCurrFingerDiffX));
        //about 35 degrees, left or right
        boolean badAngle = !(( 0.0f < angle && angle < 0.611f)
                || 2.53f < angle && angle < Math.PI);

        return badAngle;
    }


    /**
     * Return the distance in pixels from the previous shove event to the current
     * event.
     *
     * @return The current distance in pixels.
     */
    public float getShovePixelsDelta() {

        /* Prefer first finger unless 2nd finger absolute difference is overpowering.
         * This is better than oscillating between the 2 finger differences */

        float diff0 = mCurrFinger0Y - mPrevFinger0Y;
        float diff1 = mCurrFinger1Y - mPrevFinger1Y;
        if (Math.abs(diff1) > Math.abs(diff0) && Math.abs(diff0) < 2) {
            return diff1;
        } else {
            return diff0;
        }
    }

}
