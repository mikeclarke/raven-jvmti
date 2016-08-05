package com.getsentry.raven.jvmti;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.Collections;
import java.util.Map;
import java.util.HashMap;

public final class Frame {
	private static final boolean active = true;

    public static boolean isAvailable() {
        return active;
    }

    // Content access
    public Method getMethod() {
        return method;
    }

    public boolean isNative() {
        return (method.getModifiers() & Modifier.NATIVE) != 0;
    }

    public boolean couldLocalsBeLoaded() {
        return locals != null;
    }

    public Object getThis() {
        if ((method.getModifiers() & Modifier.STATIC) != 0) {
            return null;
        }
        return localThis;
    }

    public String getName(int offset) {
        if (locals == null) {
            throw new RuntimeException("Frame has no accessible locals.");
        }
        if (locals[offset] != null) {
            return locals[offset].name;
        } else {
            throw new IllegalArgumentException("Local variable " + offset
                    + " is not live.");
        }
    }

    public boolean hasLiveLocal(String name) {
        return namedLocals.containsKey(name);
    }

    public String getSignature(int offset) {
        if (locals == null) {
            throw new RuntimeException("Frame has no accessible locals.");
        }
        if (locals[offset] != null) {
            return locals[offset].signature;
        } else {
            throw new IllegalArgumentException("Local variable " + offset
                    + " is not live.");
        }
    }

    public String getSignature(String name) {
        if (locals == null) {
            throw new RuntimeException("Frame has no accessible locals.");
        }
        LocalVariable local = namedLocals.get(name);
        if (local != null) {
            return local.signature;
        } else {
            throw new IllegalArgumentException("No live local variable named "
                    + name);
        }
    }

    public String getGenericSignature(int offset) {
        if (locals == null) {
            throw new RuntimeException("Frame has no accessible locals.");
        }
        if (locals[offset] != null) {
            return locals[offset].generic_signature;
        } else {
            throw new IllegalArgumentException("Local variable " + offset
                    + " is not live.");
        }
    }

    public String getGenericSignature(String name) {
        if (locals == null) {
            throw new RuntimeException("Frame has no accessible locals.");
        }
        LocalVariable local = namedLocals.get(name);
        if (local != null) {
            return local.generic_signature;
        } else {
            throw new IllegalArgumentException("No live local variable named "
                    + name);
        }
    }

    public int getLocalsCount() {
        return locals != null ? locals.length : 0;
    }

    public boolean isLocalLive(int offset) {
        if (locals == null) {
            throw new RuntimeException("Frame has no accessible locals.");
        }
        return locals[offset].live;
    }

    public Object getLocal(int offset) {
        if (locals == null) {
            throw new RuntimeException("Frame has no accessible locals.");
        }
        if (locals[offset] != null) {
            return locals[offset].value;
        } else {
            throw new IllegalArgumentException("Local variable " + offset
                    + " is not live.");
        }
    }

    public Object getLocal(String name) {
        return null;
    }

    public Iterable<String> getLocalNames() {
        return Collections.unmodifiableSet(namedLocals.keySet());
    }

    public boolean hasLineNumber() {
        return linenumber > -1;
    }

    public int getLineNumber() {
        if (isNative()) {
            throw new RuntimeException(
                    "Cannot get line number from frame of native method.");
        } else {
            return linenumber;
        }
    }

    public int getLocation() {
        if (isNative()) {
            throw new RuntimeException(
                    "Cannot get line number from frame of native method.");
        } else {
            return location;
        }
    }

    private final Method method;
    private final Object localThis;
    public final LocalVariable[] locals;
    private final int location;
    private final int linenumber;
    public final Map<String, LocalVariable> namedLocals = new HashMap<String, LocalVariable>();

    public Frame(Method method, Object localThis, LocalVariable[] locals,
            int pos, int lineno) {
        this.method = method;
        this.localThis = localThis;
        this.locals = locals;
        this.location = pos;
        this.linenumber = lineno;
        if (locals != null) {
            for (LocalVariable local : locals) {
                if (local != null) {
                    namedLocals.put(local.name, local);
                }
            }
        }
    }

    private static final class LocalVariable {
        final boolean live;
        final String name;
        final String signature;
        final String generic_signature;
        final Object value;

        private LocalVariable(String name, String signature,
                String generic_signature, Object value) {
            this.live = true;
            this.name = name;
            this.signature = signature;
            this.generic_signature = generic_signature;
            this.value = value;
        }

        private LocalVariable(String name, String signature,
                String generic_signature) {
            this.live = false;
            this.name = name;
            this.signature = signature;
            this.generic_signature = generic_signature;
            this.value = null;
        }
    }


}
