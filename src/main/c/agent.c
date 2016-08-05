#include <jvmti.h>
#include <string.h>
#include <stdio.h>

jint throwException(JNIEnv *env, const char *name, const char *msg) {
    jclass clazz;
    clazz = (*env)->FindClass(env, name);
    return (*env)->ThrowNew(env, clazz, msg);
}

void printStackTrace(JNIEnv* env, jobject exception) {
    jclass throwable_class = (*env)->FindClass(env, "java/lang/Throwable");
    jmethodID print_method = (*env)->GetMethodID(env, throwable_class, "printStackTrace", "()V");
    (*env)->CallVoidMethod(env, exception, print_method);
}

jobject getLocalValue(jvmtiEnv* jvmti, JNIEnv *env, jthread thread, jint depth,
        jvmtiLocalVariableEntry *table, int index) {
    jobject result;
    jint iVal;
    jfloat fVal;
    jdouble dVal;
    jlong jVal;
    jvmtiError tiErr;
    jclass reflectClass;
    jmethodID valueOf;
    // Retreive
    switch (table[index].signature[0]) {
        case '[': // Array
        case 'L': // Object
            tiErr = (*jvmti)->GetLocalObject(jvmti, thread, depth, table[index].slot, &result);
            break;
        case 'J': // long
            tiErr = (*jvmti)->GetLocalLong(jvmti, thread, depth, table[index].slot, &jVal);
            break;
        case 'F': // float
            tiErr = (*jvmti)->GetLocalFloat(jvmti, thread, depth, table[index].slot, &fVal);
            break;
        case 'D': // double
            tiErr = (*jvmti)->GetLocalDouble(jvmti, thread, depth, table[index].slot, &dVal);
            break;
            // Integer types
        case 'I': // int
        case 'S': // short
        case 'C': // char
        case 'B': // byte
        case 'Z': // boolean
            tiErr = (*jvmti)->GetLocalInt(jvmti, thread, depth, table[index].slot, &iVal);
            break;
            // error type
        default:
            return NULL;
    }
    if (tiErr != JVMTI_ERROR_NONE) {
        return NULL;
    }
    // Box primitives
    switch (table[index].signature[0]) {
        case 'J': // long
            reflectClass = (*env)->FindClass(env, "java/lang/Long");
            valueOf = (*env)->GetStaticMethodID(env, reflectClass, "valueOf", "(J)Ljava/lang/Long;");
            result = (*env)->CallStaticObjectMethod(env, reflectClass, valueOf, jVal);
            break;
        case 'F': // float
            reflectClass = (*env)->FindClass(env, "java/lang/Float");
            valueOf = (*env)->GetStaticMethodID(env, reflectClass, "valueOf", "(F)Ljava/lang/Float;");
            result = (*env)->CallStaticObjectMethod(env, reflectClass, valueOf, fVal);
            break;
        case 'D': // double
            reflectClass = (*env)->FindClass(env, "java/lang/Double");
            valueOf = (*env)->GetStaticMethodID(env, reflectClass, "valueOf", "(D)Ljava/lang/Double;");
            result = (*env)->CallStaticObjectMethod(env, reflectClass, valueOf, dVal);
            break;
            // INTEGER TYPES
        case 'I': // int
            reflectClass = (*env)->FindClass(env, "java/lang/Integer");
            valueOf = (*env)->GetStaticMethodID(env, reflectClass, "valueOf", "(I)Ljava/lang/Integer;");
            result = (*env)->CallStaticObjectMethod(env, reflectClass, valueOf, iVal);
            break;
        case 'S': // short
            reflectClass = (*env)->FindClass(env, "java/lang/Short");
            valueOf = (*env)->GetStaticMethodID(env, reflectClass, "valueOf", "(S)Ljava/lang/Short;");
            result = (*env)->CallStaticObjectMethod(env, reflectClass, valueOf, iVal);
            break;
        case 'C': // char
            reflectClass = (*env)->FindClass(env, "java/lang/Character");
            valueOf = (*env)->GetStaticMethodID(env, reflectClass, "valueOf", "(C)Ljava/lang/Character;");
            result = (*env)->CallStaticObjectMethod(env, reflectClass, valueOf, iVal);
            break;
        case 'B': // byte
            reflectClass = (*env)->FindClass(env, "java/lang/Byte");
            valueOf = (*env)->GetStaticMethodID(env, reflectClass, "valueOf", "(B)Ljava/lang/Byte;");
            result = (*env)->CallStaticObjectMethod(env, reflectClass, valueOf, iVal);
            break;
        case 'Z': // boolean
            reflectClass = (*env)->FindClass(env, "java/lang/Boolean");
            valueOf = (*env)->GetStaticMethodID(env, reflectClass, "valueOf", "(Z)Ljava/lang/Boolean;");
            result = (*env)->CallStaticObjectMethod(env, reflectClass, valueOf, iVal);
            break;
        default:  // jobject
            break;
    }
    return result;
}

void makeLocalVariable(jvmtiEnv* jvmti, JNIEnv *env, jthread thread,
        jint depth, jclass localClass, jmethodID live,
        jmethodID dead, jlocation location,
        jobjectArray locals, jvmtiLocalVariableEntry *table,
        int index) {
    jstring name;
    jstring sig;
    jstring gensig;
    jobject value;
    jobject local;
    name = (*env)->NewStringUTF(env, table[index].name);
    sig = (*env)->NewStringUTF(env, table[index].signature);
    if (table[index].generic_signature) {
        gensig = (*env)->NewStringUTF(env, table[index].generic_signature);
    } else {
        gensig = NULL;
    }
    if (location >= table[index].start_location
            && location <= (table[index].start_location + table[index].length)) {
        value = getLocalValue(jvmti, env, thread, depth, table, index);
        local = (*env)->NewObject(env, localClass, live, name, sig, gensig, value);
    } else {
        local = (*env)->NewObject(env, localClass, dead, name, sig, gensig);
    }
    (*env)->SetObjectArrayElement(env, locals, index, local);
}

jobject makeFrameObject(jvmtiEnv* jvmti, JNIEnv *env, jmethodID method, jobject
        this, jobjectArray locals, jlong pos, jint lineno) {
    jvmtiError tiErr;
    jclass methodClass;
    jint modifiers;
    jobject frameMethod;
    jclass frameClass;
    jmethodID ctor;

    tiErr = (*jvmti)->GetMethodDeclaringClass(jvmti, method, &methodClass);

    if (tiErr != JVMTI_ERROR_NONE) {
        throwException(env, "java/lang/RuntimeException", "Could not get the declaring class of the method.");
        return NULL;
    }
    tiErr = (*jvmti)->GetMethodModifiers(jvmti, method, &modifiers);

    if (tiErr != JVMTI_ERROR_NONE) {
        throwException(env, "java/lang/RuntimeException", "Could not get the modifiers of the method.");
        return NULL;
    }
    frameMethod = (*env)->ToReflectedMethod(env, methodClass, method, modifiers & 8); // ACC_STATIC == 8
    if (frameMethod == NULL) {
        return NULL; // ToReflectedMethod raised an exception
    }
    frameClass = (*env)->FindClass(env, "com/getsentry/raven/jvmti/Frame");
    if (frameClass == NULL) {
        return NULL;
    }
    ctor = (*env)->GetMethodID(env, frameClass, "<init>",
            "(Ljava/lang/reflect/Method;Ljava/lang/Object;[Lcom/getsentry/raven/jvmti/Frame$LocalVariable;II)V");

    if (ctor == NULL) {
        return NULL;
    }
    return (*env)->NewObject(env, frameClass, ctor, frameMethod, this, locals, pos, lineno);
}

jobject buildFrame(jvmtiEnv* jvmti, JNIEnv *env, jthread thread, jint depth, jboolean get_locals,
        jmethodID method, jlocation location) {
    jvmtiError tiErr;
    jvmtiLocalVariableEntry *table;
    jvmtiLineNumberEntry* lnnotable;
    jint num_entries;
    jobject this;
    jobjectArray locals;
    jint lineno;
    jclass localClass;
    jmethodID liveCtor;
    jmethodID deadCtor;
    int i;
    this = NULL;
    if (get_locals) {
        tiErr = (*jvmti)->GetLocalVariableTable(jvmti, method, &num_entries, &table);
    } else {
        // If the information wasn't requested, it's absent; handle as same case
        tiErr = JVMTI_ERROR_ABSENT_INFORMATION;
    }
    if (tiErr != JVMTI_ERROR_NONE) {
        locals = NULL;
        switch(tiErr) {
            // Pass cases
            case JVMTI_ERROR_ABSENT_INFORMATION:
            case JVMTI_ERROR_NATIVE_METHOD:
                break;
                // Error cases
            case JVMTI_ERROR_MUST_POSSESS_CAPABILITY:
                throwException(env, "java/lang/RuntimeException", "access_local_variables capability not enabled.");
                return NULL;
            case JVMTI_ERROR_INVALID_METHODID:
                throwException(env, "java/lang/IllegalArgumentException", "Illegal jmethodID.");
                return NULL;
            case JVMTI_ERROR_NULL_POINTER:
                throwException(env, "java/lang/NullPointerException", "passed null to GetLocalVariableTable().");
                return NULL;
            default:
                throwException(env, "java/lang/RuntimeException", "Unknown JVMTI Error.");
                return NULL;
        }
    } else {
        localClass = (*env)->FindClass(env, "com/getsentry/raven/jvmti/Frame$LocalVariable");
        liveCtor = (*env)->GetMethodID(env, localClass, "<init>",
                "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/Object;)V");
        deadCtor = (*env)->GetMethodID(env, localClass, "<init>",
                "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
        locals = (*env)->NewObjectArray(env, num_entries, localClass, NULL);
        for (i = 0; i < num_entries; i++) {
            makeLocalVariable(jvmti, env, thread, depth, localClass, liveCtor, deadCtor, location, locals, table, i);
        }
        (*jvmti)->Deallocate(jvmti, (unsigned char *)table);
    }
    if (get_locals) {
        tiErr = (*jvmti)->GetLocalObject(jvmti, thread, depth, 0, &this);
        if (tiErr != JVMTI_ERROR_NONE) {
            this = NULL;
        }
    }
    tiErr = (*jvmti)->GetLineNumberTable(jvmti, method, &num_entries, &lnnotable);
    if (tiErr != JVMTI_ERROR_NONE) {
        lineno = -1; // Not retreived
    } else {
        for (i = 0; i < num_entries; i++) {
            if (location < lnnotable->start_location) break;
            lineno = lnnotable->line_number;
        }
        (*jvmti)->Deallocate(jvmti, (unsigned char *)lnnotable);
    }
    return makeFrameObject(jvmti, env, method, this, locals, location, lineno);
}

jobjectArray buildStackTraceFrames(jvmtiEnv* jvmti, JNIEnv *env, jthread thread,
        jint startDepth, jboolean includeLocals) {
    jclass resultClass;
    jint i;
    jvmtiFrameInfo* frames;
    jint count;
    jvmtiError tiErr;
    jobjectArray result;
    jobject frame;
    tiErr = (*jvmti)->GetFrameCount(jvmti, thread, &i);
    if (tiErr != JVMTI_ERROR_NONE) {
        throwException(env, "java/lang/RuntimeException", "Could not get the frame count.");
        return NULL;
    }
    tiErr = (*jvmti)->Allocate(jvmti, i*(int)sizeof(jvmtiFrameInfo), (unsigned char **)&frames);
    if (tiErr != JVMTI_ERROR_NONE) {
        throwException(env, "java/lang/RuntimeException", "Could not allocate frame buffer.");
        return NULL;
    }
    tiErr = (*jvmti)->GetStackTrace(jvmti, thread, startDepth, i, frames, &count);
    if (tiErr != JVMTI_ERROR_NONE) {
        (*jvmti)->Deallocate(jvmti, (unsigned char *)frames);
        throwException(env, "java/lang/RuntimeException", "Could not get stack trace.");
        return NULL;
    }
    resultClass = (*env)->FindClass(env, "com/getsentry/raven/jvmti/Frame");
    result = (*env)->NewObjectArray(env, count, resultClass, NULL);
    if (result == NULL) {
        (*jvmti)->Deallocate(jvmti, (unsigned char *)frames);
        return NULL; // OutOfMemory
    }
    for (i = 0; i < count; i++) {
        frame = buildFrame(jvmti, env, thread, startDepth + i, includeLocals, frames[i].method, frames[i].location);
        if (frame == NULL) {
            (*jvmti)->Deallocate(jvmti, (unsigned char *)frames);
            throwException(env, "java/lang/RuntimeException", "Error accessing frame object.");
            return NULL;
        }
        (*env)->SetObjectArrayElement(env, result, i, frame);
    }
    (*jvmti)->Deallocate(jvmti, (unsigned char *)frames); 
    return result;
}

void JNICALL ExceptionCallback(jvmtiEnv* jvmti, JNIEnv* env, jthread thread,
        jmethodID method, jlocation location, jobject exception,
        jmethodID catch_method, jlocation catch_location) {
    char* class_name;
    jclass exception_class = (*env)->GetObjectClass(env, exception);
    (*jvmti)->GetClassSignature(jvmti, exception_class, &class_name, NULL);

    // There are a bunch of these on load, just bail early to avoid noise
    if (strcmp(class_name, "Ljava/lang/ClassNotFoundException;") == 0) {
        return;
    }
    if (strcmp(class_name, "Ljava/io/FileNotFoundException;") == 0) {
        return;
    }
    if (strcmp(class_name, "Ljava/security/PrivilegedActionException;") == 0) {
        return;
    }

    printf ("JVMTI ExceptionCallback C Code\n");
    printf ("==============================\n");
    printStackTrace(env, exception);

    jint entry_count;
    jvmtiLocalVariableEntry *local_table;

    jvmtiError jerr;
    jerr = (*jvmti)->GetLocalVariableTable(jvmti, method, &entry_count, &local_table);

    if (jerr == JVMTI_ERROR_NONE) {
        int j;
        for (j = 0; j < entry_count; j++) {
            printf ("Slot: %d\n", local_table[j].slot);
            printf ("  Name: %s\n", local_table[j].name);
            printf ("  Sig: %s\n", local_table[j].signature);
            printf ("  Gen Sig: %s\n", local_table[j].generic_signature);
            printf ("  Start Loc: %ld\n", (long)local_table[j].start_location);
            printf ("  Length: %d\n", (int) local_table[j].length);
        }
    }

    jobjectArray result;
    jboolean get_locals = JNI_TRUE;

    char *className = "com/getsentry/raven/jvmti/LocalsCache";
    char *methodName = "setResult";
    char *descriptor = "([Lcom/getsentry/raven/jvmti/Frame;)V";

    jclass callbackClass = (*env)->FindClass(env, className);

    if (!callbackClass) {
        (*env)->ExceptionClear(env);
        fprintf(stderr,"C:\tUnable to locate callback class.\n");
        return;
    }

    jmethodID callbackMethodID = (*env)->GetStaticMethodID(env, callbackClass, methodName, descriptor);

    if (!callbackMethodID) {
        fprintf(stderr, "C:\tUnable to locate callback VMInit method\n");
        return;
    }

    jint startDepth = 0;
    result = buildStackTraceFrames(jvmti, env, thread, startDepth, get_locals);

    (*env)->CallStaticVoidMethod(env, callbackClass, callbackMethodID, result);
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM* vm, char* options, void* reserved) {
    jvmtiEnv* jvmti;
    jvmtiEventCallbacks callbacks;
    jvmtiCapabilities capabilities;

    (*vm)->GetEnv(vm, (void**)&jvmti, JVMTI_VERSION_1_0);

    memset(&capabilities, 0, sizeof(capabilities));
    capabilities.can_access_local_variables = 1;
    capabilities.can_generate_exception_events = 1;
    capabilities.can_get_line_numbers = 1;
    (*jvmti)->AddCapabilities(jvmti, &capabilities);

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.Exception = ExceptionCallback;
    (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, NULL);

    return 0;
}
