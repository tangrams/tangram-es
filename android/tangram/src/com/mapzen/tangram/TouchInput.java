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
 * {@code TouchInput} collects touch data, applies gesture detectors, resolves simultaneous
 * detection, and calls the appropriate input responders.
 */
public class TouchInput implements OnTouchListener, OnScaleGestureListener,
        OnRotateGestureListener, OnGestureListener, OnDoubleTapListener, OnShoveGestureListener {

    /**
     * List of gestures that can be detected and responded to
     */
    public enum Gestures {
        TAP,
        DOUBLE_TAP,
        LONG_PRESS,
        PAN,
        ROTATE,
        SCALE,
        SHOVE,
        ;

        /**
         * Whether a gesture uses multiple touch points simultaneously
         * @return True if the gesture is multi-touch, otherwise false
         */
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

    /**
     * Interface for responding to a tap gesture
     */
    public interface TapResponder {
        /**
         * Called immediately after a touch is lifted in a tap gesture; may be part of a double tap
         * @param x The x screen coordinate of the tapped point
         * @param y The y screen coordinate of the tapped point
         * @return True if the event is consumed, false if the event should continue to propagate
         */
        boolean onSingleTapUp(float x, float y);

        /**
         * Called after a touch is lifted and determined to not be part of a double tap
         *
         * The timeout duration for a double tap is determined by {@link ViewConfiguration}
         * @param x The x screen coordinate of the tapped point
         * @param y The y screen coordinate of the tapped point
         * @return True if the event is consumed, false if the event should continue to propagate
         */
        boolean onSingleTapConfirmed(float x, float y);
    }

    /**
     * Interface for responding to a double-tap gesture
     */
    public interface DoubleTapResponder {
        /**
         * Called immediately after the second time a touch is lifted in a double tap gesture
         *
         * The allowable duration between taps is determined by {@link ViewConfiguration}
         * @param x The x screen coordinate of the tapped point
         * @param y The y screen coordinate of the tapped point
         * @return True if the event is consumed, false if the event should continue to propagate
         */
        boolean onDoubleTap(float x, float y);
    }

    /**
     * Interface for responding to a long press gesture
     */
    public interface LongPressResponder {
        /**
         * Called immediately after a long press is detected
         *
         * The duration threshold for a long press is determined by {@link ViewConfiguration}
         * @param x The x screen coordinate of the pressed point
         * @param y The y screen coordinate of the pressed point
         */
        void onLongPress(float x, float y);
    }

    /**
     * Interface for responding to a panning (dragging) gesture
     */
    public interface PanResponder {
        /**
         * Called repeatedly while a touch point is dragged
         * @param startX The starting x screen coordinate for an interval of motion
         * @param startY The starting y screen coordinate for an interval of motion
         * @param endX The ending x screen coordinate for an interval of motion
         * @param endY The ending y screen coordinate for an interval of motion
         * @return True if the event is consumed, false if the event should continue to propagate
         */
        boolean onPan(float startX, float startY, float endX, float endY);

        /**
         * Called when a dragged touch point with non-zero velocity is lifted
         * @param posX The x screen coordinate where the touch was lifted
         * @param posY The y screen coordinate where the touch was lifted
         * @param velocityX The x velocity of the gesture in screen coordinates per second
         * @param velocityY The y velocity of the gesture in screen coordinates per second
         * @return True if the event is consumed, false if the event should continue to propagate
         */
        boolean onFling(float posX, float posY, float velocityX, float velocityY);
    }

    /**
     * Interface for responding to a scaling (pinching) gesture
     */
    public interface ScaleResponder {
        /**
         * Called repeatedly while two touch points are moved closer to or further from each other
         * @param x The x screen coordinate of the point between the two touch points
         * @param y The y screen coordinate of the point between the two touch points
         * @param scale The scale factor relative to the previous scaling event
         * @param velocity The rate of scale change in units per second
         * @return True if the event is consumed, false if the event should continue to propagate
         */
        boolean onScale(float x, float y, float scale, float velocity);
    }

    /**
     * Interface for responding to a rotation gesture
     */
    public interface RotateResponder {
        /**
         * Called repeatedly while two touch points are rotated around a point
         * @param x The x screen coordinate of the center of rotation
         * @param y The y screen coordinate of the center of rotation
         * @param rotation The change in rotation of the touch points relative to the previous
         * rotation event, in counter-clockwise radians
         * @return True if the event is consumed, false if the event should continue to propagate
         */
        boolean onRotate(float x, float y, float rotation);
    }

    /**
     * Interface for responding to a shove (two-finger vertical drag) gesture
     */
    public interface ShoveResponder {
        /**
         * Called repeatedly while two touch points are moved together vertically
         * @param distance The vertical distance moved by the two touch points relative to the last
         * shoving event, in screen coordinates
         * @return True if the event is consumed, false if the event should continue to propagate
         */
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

    /**
     * Construct a new touch input manager; this may only be called on the UI thread
     * @param context A {@link Context} whose {@code Handler} will be used for deferred events
     */
    public TouchInput(Context context) {

        this.panTapGestureDetector = new GestureDetector(context, this);
        this.scaleGestureDetector = new ScaleGestureDetector(context, this);
        this.rotateGestureDetector = new RotateGestureDetector(context, this);
        this.shoveGestureDetector = new ShoveGestureDetector(context, this);

        this.detectedGestures = EnumSet.noneOf(Gestures.class);
        this.allowedSimultaneousGestures = new EnumMap<>(Gestures.class);

        // By default, all gestures are allowed to detect simultaneously
        for (Gestures g : Gestures.values()) {
            allowedSimultaneousGestures.put(g, EnumSet.allOf(Gestures.class));
        }
    }

    /**
     * Set a {@link TapResponder}
     * @param responder The responder object, or null to leave these gesture events unchanged
     */
    public void setTapResponder(TapResponder responder) {
        this.tapResponder = responder;
    }

    /**
     * Set a {@link DoubleTapResponder}
     * @param responder The responder object, or null to leave these gesture events unchanged
     */
    public void setDoubleTapResponder(DoubleTapResponder responder) {
        this.doubleTapResponder = responder;
    }

    /**
     * Set a {@link LongPressResponder}
     * @param responder The responder object, or null to leave these gesture events unchanged
     */
    public void setLongPressResponder(LongPressResponder responder) {
        this.longPressResponder = responder;
    }

    /**
     * Set a {@link PanResponder}
     * @param responder The responder object, or null to leave these gesture events unchanged
     */
    public void setPanResponder(PanResponder responder) {
        this.panResponder = responder;
    }

    /**
     * Set a {@link ScaleResponder}
     * @param responder The responder object, or null to leave these gesture events unchanged
     */
    public void setScaleResponder(ScaleResponder responder) {
        this.scaleResponder = responder;
    }

    /**
     * Set a {@link RotateResponder}
     * @param responder The responder object, or null to leave these gesture events unchanged
     */
    public void setRotateResponder(RotateResponder responder) {
        this.rotateResponder = responder;
    }

    /**
     * Set a {@link ShoveResponder}
     * @param responder The responder object, or null to leave these gesture events unchanged
     */
    public void setShoveResponder(ShoveResponder responder) {
        this.shoveResponder = responder;
    }

    /**
     * Set whether the gesture {@code second} can be recognized while {@code first} is in progress
     * @param first Initial gesture type
     * @param second Subsequent gesture type
     * @param allowed True if {@code second} should be recognized, else false
     */
    public void setSimultaneousDetectionAllowed(Gestures first, Gestures second, boolean allowed) {
        if (first != second) {
            if (allowed) {
                allowedSimultaneousGestures.get(second).add(first);
            } else {
                allowedSimultaneousGestures.get(second).remove(first);
            }
        }
    }

    /**
     * Get whether the gesture {@code second} can be recognized while {@code first} is in progress
     * @param first Initial gesture type
     * @param second Subsequent gesture type
     * @return True if {@code second} will be recognized, else false
     */
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

    @Override
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
