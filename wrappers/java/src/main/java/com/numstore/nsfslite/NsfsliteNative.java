package com.numstore.nsfslite;

/**
 * Native JNI interface to the nsfslite C library.
 * This class contains all the native method declarations.
 * DO NOT use this class directly - use the Nsfslite class instead.
 */
class NsfsliteNative {

    static {
        // Load the native library
        System.loadLibrary("nsfslite_jni");
    }

    // Lifecycle
    static native long open(String dbPath, String walPath);
    static native void close(long handle);

    // Higher Order Operations
    static native long createVariable(long handle, String name);  // nsfslite_new
    static native long getVariable(long handle, String name);      // nsfslite_get_id
    static native void deleteVariable(long handle, String name);   // nsfslite_delete
    static native long variableLength(long handle, long varId);    // nsfslite_fsize

    // Insert - simplified version
    static native void insert(long handle, long varId, long offset, byte[] data);
    static native void insertTx(long handle, long varId, long txnHandle, long offset, byte[] data);

    // Read/Write/Remove with stride
    static native byte[] read(long handle, long varId, long start, long stop, long step);
    static native void write(long handle, long varId, long start, long stop, long step, byte[] data);
    static native void writeTx(long handle, long varId, long txnHandle, long start, long stop, long step, byte[] data);
    static native byte[] remove(long handle, long varId, long start, long stop, long step, boolean returnData);
    static native byte[] removeTx(long handle, long varId, long txnHandle, long start, long stop, long step, boolean returnData);

    // Transaction Management
    static native long beginTransaction(long handle);
    static native void commitTransaction(long handle, long txnHandle);
}
