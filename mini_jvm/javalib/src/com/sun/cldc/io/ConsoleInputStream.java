package com.sun.cldc.io;

import java.io.IOException;
import java.io.InputStream;

public final class ConsoleInputStream extends InputStream {
    public int read() throws IOException {
        return readConsole();
    }

    private native int readConsole() throws IOException;
}
