package com.mapzen.tangram;

/**
 * Holds four integer values that adjust the bounds of a rectangle. Positive values move edges
 * towards the center of the rectangle.
 */
public class EdgePadding {
    /**
     * Padding from left edge.
     */
    public int left;

    /**
     * Padding from top edge.
     */
    public int top;

    /**
     * Padding from right edge.
     */
    public int right;

    /**
     * Padding from bottom edge.
     */
    public int bottom;

    /**
     * Create an instance with values of 0.
     */
    public EdgePadding() {
        this(0, 0, 0, 0);
    }

    /**
     * Create an instance with the given values.
     * @param left Padding from left edge.
     * @param top Padding from top edge.
     * @param right Padding from right edge.
     * @param bottom Padding from bottom edge.
     */
    public EdgePadding(final int left, final int top, final int right, final int bottom) {
        this.left = left;
        this.top = top;
        this.right = right;
        this.bottom = bottom;
    }

    /**
     * Create an instance by copying the values of another instance.
     * @param other Instance to copy values from.
     */
    public EdgePadding(final EdgePadding other) {
        this.left = other.left;
        this.top = other.top;
        this.right = other.right;
        this.bottom = other.bottom;
    }

    public boolean equals(final Object other) {
        if (other instanceof EdgePadding) {
            EdgePadding otherPadding = (EdgePadding) other;
            return left == otherPadding.left
                    && top == otherPadding.top
                    && right == otherPadding.right
                    && bottom == otherPadding.bottom;
        }
        return false;
    }
}
