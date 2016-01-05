package com.mapzen.tangram;

import android.content.Context;
import android.os.SystemClock;
import android.view.GestureDetector;
import android.view.GestureDetector.OnDoubleTapListener;
import android.view.GestureDetector.OnGestureListener;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.ScaleGestureDetector.OnScaleGestureListener;
import android.view.View;
import android.view.View.OnTouchListener;

import com.almeros.android.multitouch.RotateGestureDetector;
import com.almeros.android.multitouch.RotateGestureDetector.OnRotateGestureListener;
import com.almeros.android.multitouch.ShoveGestureDetector;
import com.almeros.android.multitouch.ShoveGestureDetector.OnShoveGestureListener;

import java.util.Arrays;

/**
 * Collects touch data, applies gesture detectors, resolves simultaneous detection, and calls the
 * appropriate input responders
 */
public class TouchManager implements OnTouchListener, OnScaleGestureListener,
        OnRotateGestureListener, OnGestureListener, OnDoubleTapListener, OnShoveGestureListener {

    public enum Gestures {
        TAP,
        DOUBLE_TAP,
        LONG_PRESS,
        PAN,
        ROTATE,
        SCALE,
        SHOVE,
        ;

        public boolean isMultiTouch() {
            switch(this) {
                case ROTATE:
                case SCALE:
                case SHOVE:
                    return true;
                default:
                    return false;
            }
        }
    }

    public interface TapResponder {
        boolean onSingleTapUp(float x, float y);
        boolean onSingleTapConfirmed(float x, float y);
    }

    public interface DoubleTapResponder {
        boolean onDoubleTap(float x, float y);
    }

    public interface LongPressResponder {
        void onLongPress(float x, float y);
    }

    public interface PanResponder {
        boolean onPan(float startX, float startY, float endX, float endY);
    }

    public interface ScaleResponder {
        boolean onScale(float x, float y, float scale, float velocity);
    }

    public interface RotateResponder {
        boolean onRotate(float x, float y, float rotation);
    }

    public interface ShoveResponder {
        boolean onShove(float distance);
    }

    private static final long MULTITOUCH_BUFFER_TIME = 256; // milliseconds

    private GestureDetector panTapGestureDetector;
    private ScaleGestureDetector scaleGestureDetector;
    private RotateGestureDetector rotateGestureDetector;
    private ShoveGestureDetector shoveGestureDetector;

    private TapResponder tapResponder;
    private DoubleTapResponder doubleTapResponder;
    private LongPressResponder longPressResponder;
    private PanResponder panResponder;
    private ScaleResponder scaleResponder;
    private RotateResponder rotateResponder;
    private ShoveResponder shoveResponder;

    private boolean[] detectedGestures;
    private boolean[][] allowedSimultaneousGestures;

    private long lastMultiTouchEndTime = -MULTITOUCH_BUFFER_TIME;

    public TouchManager(Context context) {

        this.panTapGestureDetector = new GestureDetector(context, this);
        this.scaleGestureDetector = new ScaleGestureDetector(context, this);
        this.rotateGestureDetector = new RotateGestureDetector(context, this);
        this.shoveGestureDetector = new ShoveGestureDetector(context, this);

        int nGestures = Gestures.values().length;
        this.detectedGestures = new boolean[nGestures];
        this.allowedSimultaneousGestures = new boolean[nGestures][nGestures];

        // By default, all gestures are allowed to detect simultaneously
        for (boolean[] arr : allowedSimultaneousGestures) {
            Arrays.fill(arr, true);
        }
    }

    public void setTapResponder(TapResponder responder) {
        this.tapResponder = responder;
    }

    public void setDoubleTapResponder(DoubleTapResponder responder) {
        this.doubleTapResponder = responder;
    }

    public void setLongPressResponder(LongPressResponder responder) {
        this.longPressResponder = responder;
    }

    public void setPanResponder(PanResponder responder) {
        this.panResponder = responder;
    }

    public void setScaleResponder(ScaleResponder responder) {
        this.scaleResponder = responder;
    }

    public void setRotateResponder(RotateResponder responder) {
        this.rotateResponder = responder;
    }

    public void setShoveResponder(ShoveResponder responder) {
        this.shoveResponder = responder;
    }

    // Set whether 'second' can detect while 'first' is in progress
    public void setSimultaneousDetectionAllowed(Gestures first, Gestures second, boolean allowed) {
        if (first != second) {
            allowedSimultaneousGestures[second.ordinal()][first.ordinal()] = allowed;
        }
    }

    public boolean isSimultaneousDetectionAllowed(Gestures first, Gestures second) {
        return allowedSimultaneousGestures[second.ordinal()][first.ordinal()];
    }

    private boolean isDetectionAllowed(Gestures g) {
        boolean [] allowed = allowedSimultaneousGestures[g.ordinal()];
        // TODO: Replace boolean array with bitmask
        for (int i = 0; i < allowed.length; ++i) {
            if (!allowed[i] && detectedGestures[i]) {
                return false;
            }
        }
        if (!g.isMultiTouch()) {
            // Return false if a multitouch gesture has finished within a time threshold
            long t = SystemClock.uptimeMillis() - lastMultiTouchEndTime;
            if (t < MULTITOUCH_BUFFER_TIME) {
                return false;
            }
        }
        return true;
    }

    private void setGestureDetected(Gestures g, boolean detected) {
        detectedGestures[g.ordinal()] = detected;
        if (!detected && g.isMultiTouch()) {
            lastMultiTouchEndTime = SystemClock.uptimeMillis();
        }
    }

    // View.OnTouchListener implementation
    // ===================================

    public boolean onTouch(View v, MotionEvent event) {

        panTapGestureDetector.onTouchEvent(event);
        scaleGestureDetector.onTouchEvent(event);
        shoveGestureDetector.onTouchEvent(event);
        rotateGestureDetector.onTouchEvent(event);

        return true;
    }

    // GestureDetector.OnDoubleTapListener implementation
    // ==================================================

    @Override
    public boolean onSingleTapConfirmed(MotionEvent e) {
        if (isDetectionAllowed(Gestures.TAP) && tapResponder != null) {
            return tapResponder.onSingleTapConfirmed(e.getX(), e.getY());
        }
        return false;
    }

    @Override
    public boolean onDoubleTap(MotionEvent e) {
        if (isDetectionAllowed(Gestures.DOUBLE_TAP) && doubleTapResponder != null) {
            return doubleTapResponder.onDoubleTap(e.getX(), e.getY());
        }
        return false;
    }

    @Override
    public boolean onDoubleTapEvent(MotionEvent e) {
        return false;
    }

    // GestureDetector.OnGestureListener implementation
    // ================================================

    @Override
    public boolean onDown(MotionEvent e) {
        // When new touch is placed, dispatch a zero-distance pan;
        // this provides an opportunity to halt any current motion.
        if (isDetectionAllowed(Gestures.PAN) && panResponder != null) {
            final float x = e.getX();
            final float y = e.getY();
            return panResponder.onPan(x, y, x, y);
        }
        return false;
    }

    @Override
    public void onShowPress(MotionEvent e) {
        // Ignored
    }

    @Override
    public boolean onSingleTapUp(MotionEvent e) {
        if (isDetectionAllowed(Gestures.TAP) && tapResponder != null) {
            return tapResponder.onSingleTapUp(e.getX(), e.getY());
        }
        return false;
    }

    @Override
    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
        if (isDetectionAllowed(Gestures.PAN)) {
            int action = e2.getAction();
            boolean detected = !(action == MotionEvent.ACTION_CANCEL || action == MotionEvent.ACTION_UP);
            setGestureDetected(Gestures.PAN, detected);

            if (panResponder == null) {
                return false;
            }

            // TODO: Predictive panning
            // Use estimated velocity to counteract input->render lag

            float x = 0, y = 0;
            int n = e2.getPointerCount();
            for (int i = 0; i < n; i++) {
                x += e2.getX(i) / n;
                y += e2.getY(i) / n;
            }
            return panResponder.onPan(x + distanceX, y + distanceY, x, y);
        }
        return false;
    }

    @Override
    public void onLongPress(MotionEvent e) {
        if (isDetectionAllowed(Gestures.LONG_PRESS) && longPressResponder != null) {
            longPressResponder.onLongPress(e.getX(), e.getY());
        }
    }

    @Override
    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
        return false;
    }

    // RotateGestureDetector.OnRotateGestureListener implementation
    // ============================================================

    @Override
    public boolean onRotate(RotateGestureDetector detector) {
        if (isDetectionAllowed(Gestures.ROTATE) && rotateResponder != null) {
            float rotation = -detector.getRotationRadiansDelta();
            float x = detector.getFocusX();
            float y = detector.getFocusY();
            return rotateResponder.onRotate(x, y, rotation);
        }
        return false;
    }

    @Override
    public boolean onRotateBegin(RotateGestureDetector detector) {
        if (isDetectionAllowed(Gestures.ROTATE)) {
            setGestureDetected(Gestures.ROTATE, true);
        }
        return true;
    }

    @Override
    public void onRotateEnd(RotateGestureDetector detector) {
        setGestureDetected(Gestures.ROTATE, false);
    }

    // ScaleGestureDetector.OnScaleGestureListener implementation
    // ==========================================================

    @Override
    public boolean onScale(ScaleGestureDetector detector) {
        if (isDetectionAllowed(Gestures.SCALE) && scaleResponder != null) {
            long ms = detector.getTimeDelta();
            float dt = ms > 0 ? ms / 1000.f : 1.f;
            float scale = detector.getScaleFactor();
            float velocity = (scale - 1.f) / dt;
            float x = detector.getFocusX();
            float y = detector.getFocusY();
            return scaleResponder.onScale(x, y, scale, velocity);
        }
        return false;
    }

    @Override
    public boolean onScaleBegin(ScaleGestureDetector detector) {
        if (isDetectionAllowed(Gestures.SCALE)) {
            setGestureDetected(Gestures.SCALE, true);
        }
        return true;
    }

    @Override
    public void onScaleEnd(ScaleGestureDetector detector) {
        setGestureDetected(Gestures.SCALE, false);
    }

    // ShoveGestureDetector.OnShoveGestureListener implementation
    // ==========================================================

    @Override
    public boolean onShove(ShoveGestureDetector detector) {
        if (isDetectionAllowed(Gestures.SHOVE) && shoveResponder != null) {
            return shoveResponder.onShove(detector.getShovePixelsDelta());
        }
        return false;
    }

    @Override
    public boolean onShoveBegin(ShoveGestureDetector detector) {
        if (isDetectionAllowed(Gestures.SHOVE)) {
            setGestureDetected(Gestures.SHOVE, true);
        }
        return true;
    }

    @Override
    public void onShoveEnd(ShoveGestureDetector detector) {
        setGestureDetected(Gestures.SHOVE, false);
    }
}
