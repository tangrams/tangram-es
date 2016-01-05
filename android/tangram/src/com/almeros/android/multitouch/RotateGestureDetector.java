package com.almeros.android.multitouch;

import android.content.Context;
import android.view.MotionEvent;

/**
 * @author Almer Thie (code.almeros.com)
 * Copyright (c) 2013, Almer Thie (code.almeros.com)
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
public class RotateGestureDetector extends TwoFingerGestureDetector {

    /**
     * Listener which must be implemented which is used by RotateGestureDetector
     * to perform callbacks to any implementing class which is registered to a
     * RotateGestureDetector via the constructor.
     *
     * @see RotateGestureDetector.SimpleOnRotateGestureListener
     */
    public interface OnRotateGestureListener {
        boolean onRotate(RotateGestureDetector detector);
        boolean onRotateBegin(RotateGestureDetector detector);
        void onRotateEnd(RotateGestureDetector detector);
    }

    /**
     * Helper class which may be extended and where the methods may be
     * implemented. This way it is not necessary to implement all methods
     * of OnRotateGestureListener.
     */
    public static class SimpleOnRotateGestureListener implements OnRotateGestureListener {
        public boolean onRotate(RotateGestureDetector detector) {
            return false;
        }

        public boolean onRotateBegin(RotateGestureDetector detector) {
            return true;
        }

        public void onRotateEnd(RotateGestureDetector detector) {
            // Do nothing, overridden implementation may be used
        }
    }

    private static final float ROTATION_THRESHOLD = 0.25f; // Minimum radians of rotation to recognize

    private final OnRotateGestureListener mListener;
    private boolean mSloppyGesture;
    private boolean mRecognized;

    private float mTotalRotation;
    private float mFocusX;
    private float mFocusY;

    public RotateGestureDetector(Context context, OnRotateGestureListener listener) {
        super(context);
        mListener = listener;
    }

    private void determineFocusPoint(MotionEvent curr) {
        mFocusX = (curr.getX(0) + curr.getX(1)) * 0.5f;
        mFocusY = (curr.getY(0) + curr.getY(1)) * 0.5f;
    }

    @Override
    protected void handleStartProgressEvent(int actionCode, MotionEvent event) {
        switch (actionCode) {
            case MotionEvent.ACTION_POINTER_DOWN:
                // At least the second finger is on screen now

                resetState(); // In case we missed an UP/CANCEL event
                mPrevEvent = MotionEvent.obtain(event);
                mTimeDelta = 0;

                updateStateByEvent(event);

                // See if we have a sloppy gesture
                mSloppyGesture = isSloppyGesture(event);
                if (!mSloppyGesture) {
                    // No, start listening for gesture now
                    mGestureInProgress = true;
                }
                break;

            case MotionEvent.ACTION_MOVE:
                if (!mSloppyGesture) {
                    break;
                }

                // See if we still have a sloppy gesture
                mSloppyGesture = isSloppyGesture(event);
                if (!mSloppyGesture) {
                    // No, start listening for gesture now
                    mGestureInProgress = true;
                }
                break;
        }
    }


    @Override
    protected void handleInProgressEvent(int actionCode, MotionEvent event) {
        switch (actionCode) {
            case MotionEvent.ACTION_POINTER_UP:
                // Gesture ended with up event, update and finalize state
                updateStateByEvent(event);

                if (!mSloppyGesture && mRecognized) {
                    mListener.onRotateEnd(this);
                }

                resetState();
                break;

            case MotionEvent.ACTION_CANCEL:
                // Gesture ended with no up event, finalize state
                if (!mSloppyGesture && mRecognized) {
                    mListener.onRotateEnd(this);
                }

                resetState();
                break;

            case MotionEvent.ACTION_MOVE:
                updateStateByEvent(event);

                // Only accept the event if our relative pressure is within
                // a certain limit. This can help filter shaky data as a
                // finger is lifted.
                if (mCurrPressure / mPrevPressure > PRESSURE_THRESHOLD) {
                    determineFocusPoint(event);
                    boolean updatePrevious;
                    if (mRecognized) {
                        updatePrevious = mListener.onRotate(this);
                    } else {
                        updatePrevious = true;
                        mRecognized =
                                Math.abs(mTotalRotation) >= ROTATION_THRESHOLD &&
                                mListener.onRotateBegin(this);
                    }
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
        mRecognized = false;
        mTotalRotation = 0;
    }

    @Override
    protected void updateStateByEvent(MotionEvent event) {
        super.updateStateByEvent(event);
        mTotalRotation += getRotationRadiansDelta();
    }

    /**
     */
    public float getFocusX() {
        return mFocusX;
    }


    /**
     */
    public float getFocusY() {
        return mFocusY;
    }

    /**
     * Return the rotation difference from the previous rotate event to the current
     * event. (radians)
     *
     * @return The current rotation //difference in degrees.
     */
    public float getRotationRadiansDelta() {
        double diffRadians = Math.atan2(mPrevFingerDiffY, mPrevFingerDiffX) - Math.atan2(mCurrFingerDiffY, mCurrFingerDiffX);
        return (float) (diffRadians);
    }

    /**
     * Return the rotation difference from the previous rotate event to the current
     * event. (degrees)
     *
     * @return The current rotation //difference in degrees.
     */
    public float getRotationDegreesDelta() {
        double diffRadians = Math.atan2(mPrevFingerDiffY, mPrevFingerDiffX) - Math.atan2(mCurrFingerDiffY, mCurrFingerDiffX);
        return (float) (diffRadians * 180 / Math.PI);
    }
}
