package com.getsentry.raven.jvmti;

import com.getsentry.raven.jvmti.Frame;

public class LocalsCache {

    private static Frame[] result;

    public static void setResult(Frame[] r) {
        result = r;
    }

    public static Frame[] getResult() {
        return result;
    }

}
