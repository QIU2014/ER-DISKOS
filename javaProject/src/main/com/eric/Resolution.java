package com.eric;

public final class Resolution {
    private Resolution() {
    }

    public static native boolean set(int width, int height);
}
