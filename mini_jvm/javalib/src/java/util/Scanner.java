package java.util;

import java.io.IOException;
import java.io.InputStream;

public final class Scanner {
    private final InputStream source;
    private boolean closed;

    public Scanner(InputStream source) {
        if (source == null) {
            throw new NullPointerException();
        }
        this.source = source;
    }

    public String nextLine() {
        if (closed) {
            throw new IllegalStateException("Scanner is closed");
        }

        StringBuilder line = new StringBuilder();
        for (;;) {
            int character;
            try {
                character = source.read();
            } catch (IOException exception) {
                throw new IllegalStateException("Unable to read console input");
            }

            if (character < 0) {
                if (line.length() == 0) {
                    throw new NoSuchElementException();
                }
                return line.toString();
            }
            if (character == '\n' || character == '\r') {
                return line.toString();
            }
            if (character == '\b' || character == 0x7f) {
                if (line.length() > 0) {
                    line.setLength(line.length() - 1);
                }
                continue;
            }
            line.append((char) character);
        }
    }

    public void close() {
        closed = true;
    }
}
