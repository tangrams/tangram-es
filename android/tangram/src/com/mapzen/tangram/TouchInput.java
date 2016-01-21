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
import android.view.ViewConfiguration;

import com.almeros.android.multitouch.RotateGestureDetector;
import com.almeros.android.multitouch.RotateGestureDetector.OnRotateGestureListener;
import com.almeros.android.multitouch.ShoveGestureDetector;
import com.almeros.android.multitouch.ShoveGestureDetector.OnShoveGestureListener;

import java.util.EnumMap;
import java.util.EnumSet;

/**
 * Collects touch data, applies gesture detectors, resolves simultaneous detection, and calls the
 * appropriate input responders
 */
public class TouchInput implements OnTouchListener, OnScaleGestureListener,
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
        boolean onFling(float posX, float posY, float velocityX, float velocityY);
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
    private static final long DOUBLE_TAP_TIMEOUT = ViewConfiguration.getDoubleTapTimeout(); // milliseconds

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

    private EnumSet<Gestures> detectedGestures;
    private EnumMap<Gestures, EnumSet<Gestures>> allowedSimultaneousGestures;

    private long lastMultiTouchEndTime = -MULTITOUCH_BUFFER_TIME;

    public TouchInput(Context context) {

        this.panTapGestureDetector = new GestureDetector(context, this);
        this.scaleGestureDetector = new ScaleGestureDetector(context, this);
        this.rotateGestureDetector = new RotateGestureDetector(context, this);
        this.shoveGestureDetector = new ShoveGestureDetector(context, this);

        this.detectedGestures = EnumSet.noneOf(Gestures.class);
        this.allowedSimultaneousGestures = new EnumMap<Gestures, EnumSet<Gestures>>(Gestures.class);

        // By default, all gestures are allowed to detect simultaneously
        for (Gestures g : Gestures.values()) {
            allowedSimultaneousGestures.put(g, EnumSet.allOf(Gestures.class));
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
            if (allowed) {
                allowedSimultaneousGestures.get(second).add(first);
            } else {
                allowedSimultaneousGestures.get(second).remove(first);
            }
        }
    }

    public boolean isSimultaneousDetectionAllowed(Gestures first, Gestures second) {
        return allowedSimultaneousGestures.get(second).contains(first);
    }

    private boolean isDetectionAllowed(Gestures g) {
        if (!allowedSimultaneousGestures.get(g).containsAll(detectedGestures)) {
            return false;
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
        if (detected) {
            detectedGestures.add(g);
        } else {
            detectedGestures.remove(g);
        }
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
        // This event handles the second 'down' of a double tap, which is not a confirmed double tap
        // (e.g. it could be the start of a 'quick scale' gesture). We ignore this callback and
        // check for the 'up' event that follows.
        return false;
    }

    @Override
    public boolean onDoubleTapEvent(MotionEvent e) {
        int action = e.getActionMasked();
        long time = e.getEventTime() - e.getDownTime();
        if (action != MotionEvent.ACTION_UP || time > DOUBLE_TAP_TIMEOUT) {
            // The detector sends back only the first 'down' and the second 'up' so we only need to
            // respond when we receive an 'up' action. We also discard the gesture if the second tap
            // lasts longer than the permitted duration between taps.
            return false;
        }
        if (isDetectionAllowed(Gestures.DOUBLE_TAP) && doubleTapResponder != null) {
            return doubleTapResponder.onDoubleTap(e.getX(), e.getY());
        }
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
            int action = e2.getActionMasked();
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
        if (isDetectionAllowed(Gestures.PAN) && panResponder != null) {
            return panResponder.onFling(e2.getX(), e2.getY(), velocityX, velocityY);
        }
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
