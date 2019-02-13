package com.mapzen.tangram.android;

import android.opengl.GLES20;
import android.util.Log;

import com.mapzen.tangram.CustomRenderer;

import java.nio.FloatBuffer;

public class OverlayRenderer implements CustomRenderer {

    private int[] mVertexBufferHandle = new int[1];
    private int mPositionAttributeLocation = 0;
    private int mShaderProgramHandle = 0;
    private int mFragmentShaderHandle = 0;
    private int mVertexShaderHandle = 0;

    private final static String vertexShaderSource =
            "attribute vec2 a_position;\n" +
            "varying vec4 v_color;\n" +
            "void main() {\n" +
            "    v_color = vec4(0., 1., 0., 1.);\n" +
            "    gl_Position = vec4(a_position.x, a_position.y, 0., 1.);\n" +
            "}";

    private final static String fragmentShaderSource =
            "varying vec4 v_color;\n" +
            "void main() {\n" +
            "    gl_FragColor = v_color;\n" +
            "}";

    void logGlErrors(String location) {
        int error = GLES20.glGetError();
        String message;
        while (error != GLES20.GL_NO_ERROR) {
            switch (error) {
                case GLES20.GL_INVALID_ENUM:
                    message = "GL_INVALID_ENUM";
                    break;
                case GLES20.GL_INVALID_VALUE:
                    message = "GL_INVALID_VALUE";
                    break;
                case GLES20.GL_INVALID_OPERATION:
                    message = "GL_INVALID_OPERATION";
                    break;
                case GLES20.GL_OUT_OF_MEMORY:
                    message = "GL_OUT_OF_MEMORY";
                    break;
                case GLES20.GL_INVALID_FRAMEBUFFER_OPERATION:
                    message = "GL_INVALID_FRAMEBUFFER_OPERATION";
                    break;
                default:
                    message = "???";
                    break;
            }
            Log.e("Tangram OverlayRenderer", location + " - " + message);
            error = GLES20.glGetError();
        }
    }
    @Override
    public void initialize() {
        logGlErrors("pre-initialize");
        mVertexShaderHandle = GLES20.glCreateShader(GLES20.GL_VERTEX_SHADER);
        GLES20.glShaderSource(mVertexShaderHandle, vertexShaderSource);
        GLES20.glCompileShader(mVertexShaderHandle);

        mFragmentShaderHandle = GLES20.glCreateShader(GLES20.GL_FRAGMENT_SHADER);
        GLES20.glShaderSource(mFragmentShaderHandle, fragmentShaderSource);
        GLES20.glCompileShader(mFragmentShaderHandle);

        mShaderProgramHandle = GLES20.glCreateProgram();
        GLES20.glAttachShader(mShaderProgramHandle, mVertexShaderHandle);
        GLES20.glAttachShader(mShaderProgramHandle, mFragmentShaderHandle);
        GLES20.glLinkProgram(mShaderProgramHandle);

        logGlErrors("post-initialize-shaders");

        mPositionAttributeLocation = GLES20.glGetAttribLocation(mShaderProgramHandle, "a_position");

        GLES20.glGenBuffers(1, mVertexBufferHandle, 0);
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, mVertexBufferHandle[0]);
        GLES20.glVertexAttribPointer(mPositionAttributeLocation, 2, GLES20.GL_FLOAT, false, 8, 0);
        GLES20.glEnableVertexAttribArray(mPositionAttributeLocation);

        float[] vertices = new float[] {
                -0.5f, -0.5f,
                 0.5f, -0.5f,
                -0.5f,  0.5f,
                 0.5f, -0.5f,
                 0.5f,  0.5f,
                -0.5f,  0.5f,
        };

        GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, vertices.length * 4, FloatBuffer.wrap(vertices), GLES20.GL_STATIC_DRAW);

        logGlErrors("post-initialize");
    }

    @Override
    public void render(double width, double height, double longitude, double latitude, double zoom,
                       double rotation, double tilt, double fieldOfView) {

        logGlErrors("pre-render");

        GLES20.glDisable(GLES20.GL_DEPTH_TEST);
        GLES20.glDisable(GLES20.GL_BLEND);

        GLES20.glUseProgram(mShaderProgramHandle);
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, mVertexBufferHandle[0]);
        GLES20.glVertexAttribPointer(mPositionAttributeLocation, 2, GLES20.GL_FLOAT, false, 8, 0);
        GLES20.glEnableVertexAttribArray(mPositionAttributeLocation);

        GLES20.glDrawArrays(GLES20.GL_TRIANGLES, 0, 6);

        logGlErrors("post-render");
    }

    @Override
    public void deinitialize() {
        logGlErrors("pre-deinitialize");

        GLES20.glDeleteProgram(mShaderProgramHandle);
        mShaderProgramHandle = 0;
        GLES20.glDeleteShader(mVertexShaderHandle);
        mVertexShaderHandle = 0;
        GLES20.glDeleteShader(mFragmentShaderHandle);
        mFragmentShaderHandle = 0;
        GLES20.glDeleteBuffers(1, mVertexBufferHandle, 0);
        mVertexBufferHandle[0] = 0;

        logGlErrors("post-deinitialize");
    }
}
