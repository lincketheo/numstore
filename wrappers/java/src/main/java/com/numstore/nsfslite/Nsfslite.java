package com.numstore.nsfslite;

/**
 * NumStore File System Lite (nsfslite) - Java API
 *
 * This API closely mirrors the C API for nsfslite.
 * All methods are static and operate on opaque handles (long values).
 *
 * Example usage:
 * <pre>
 * {@code
 * long handle = Nsfslite.open("data.db", "data.wal");
 * long varId = Nsfslite.newVar(handle, "myvar");
 *
 * byte[] data = "Hello, World!".getBytes();
 * Nsfslite.insert(handle, varId, 0, data);
 *
 * byte[] read = Nsfslite.read(handle, varId, 0, 13, 1);
 * System.out.println(new String(read));
 *
 * Nsfslite.close(handle);
 * }
 * </pre>
 */
public final class Nsfslite {

    // Private constructor to prevent instantiation
    private Nsfslite() {
        throw new AssertionError("Nsfslite is a utility class and should not be instantiated");
    }

    ////////////////////////////////////////////
    // Lifecycle

    /**
     * Open a connection to an nsfslite database.
     * Equivalent to: nsfslite *nsfslite_open(const char *fname, const char *recovery_fname)
     *
     * @param dbPath path to the database file
     * @param walPath path to the WAL (recovery) file
     * @return handle to the database connection
     */
    public static long open(String dbPath, String walPath) {
        if (dbPath == null || dbPath.isEmpty()) {
            throw new IllegalArgumentException("Database path cannot be null or empty");
        }
        if (walPath == null || walPath.isEmpty()) {
            throw new IllegalArgumentException("WAL path cannot be null or empty");
        }
        return NsfsliteNative.open(dbPath, walPath);
    }

    /**
     * Close a database connection.
     * Equivalent to: int nsfslite_close(nsfslite *n)
     *
     * @param handle the database handle
     */
    public static void close(long handle) {
        NsfsliteNative.close(handle);
    }

    ////////////////////////////////////////////
    // Higher Order Operations

    /**
     * Create a new variable with the specified name.
     * Equivalent to: int64_t nsfslite_new(nsfslite *n, const char *name)
     *
     * @param handle the database handle
     * @param name the name of the variable
     * @return the variable ID
     */
    public static long newVar(long handle, String name) {
        return NsfsliteNative.createVariable(handle, name);
    }

    /**
     * Get the ID of an existing variable by name.
     * Equivalent to: int64_t nsfslite_get_id(nsfslite *n, const char *name)
     *
     * @param handle the database handle
     * @param name the name of the variable
     * @return the variable ID
     */
    public static long getId(long handle, String name) {
        return NsfsliteNative.getVariable(handle, name);
    }

    /**
     * Delete a variable by name.
     * Equivalent to: int nsfslite_delete(nsfslite *n, const char *name)
     *
     * @param handle the database handle
     * @param name the name of the variable to delete
     */
    public static void delete(long handle, String name) {
        NsfsliteNative.deleteVariable(handle, name);
    }

    /**
     * Get the size of a variable in bytes.
     * Equivalent to: size_t nsfslite_fsize(nsfslite *n, uint64_t id)
     *
     * @param handle the database handle
     * @param id the variable ID
     * @return the size in bytes
     */
    public static long fsize(long handle, long id) {
        return NsfsliteNative.variableLength(handle, id);
    }

    ////////////////////////////////////////////
    // Variable Operations

    /**
     * Insert data at the specified byte offset.
     * Simplified version of: ssize_t nsfslite_insert(nsfslite *n, uint64_t id, nsfslite_txn *tx,
     *                                                 const void *src, size_t bofst, size_t size, size_t nelem)
     *
     * @param handle the database handle
     * @param id the variable ID
     * @param offset the byte offset at which to insert
     * @param data the data to insert
     */
    public static void insert(long handle, long id, long offset, byte[] data) {
        NsfsliteNative.insert(handle, id, offset, data);
    }

    /**
     * Insert data at the specified byte offset within a transaction.
     *
     * @param handle the database handle
     * @param id the variable ID
     * @param txn the transaction handle
     * @param offset the byte offset at which to insert
     * @param data the data to insert
     */
    public static void insert(long handle, long id, long txn, long offset, byte[] data) {
        NsfsliteNative.insertTx(handle, id, txn, offset, data);
    }

    /**
     * Read data from a variable using a slice pattern [start:stop:step].
     * Equivalent to: ssize_t nsfslite_read(nsfslite *n, uint64_t id, void *dest, size_t size,
     *                                       struct nsfslite_stride stride)
     *
     * @param handle the database handle
     * @param id the variable ID
     * @param start the starting byte offset
     * @param stop the ending byte offset (exclusive)
     * @param step the step size in bytes
     * @return the data read as a byte array
     */
    public static byte[] read(long handle, long id, long start, long stop, long step) {
        return NsfsliteNative.read(handle, id, start, stop, step);
    }

    /**
     * Read data from a variable with step size of 1.
     *
     * @param handle the database handle
     * @param id the variable ID
     * @param start the starting byte offset
     * @param stop the ending byte offset (exclusive)
     * @return the data read as a byte array
     */
    public static byte[] read(long handle, long id, long start, long stop) {
        return read(handle, id, start, stop, 1);
    }

    /**
     * Write data to a variable using a slice pattern [start:stop:step].
     * Equivalent to: ssize_t nsfslite_write(nsfslite *n, uint64_t id, nsfslite_txn *tx,
     *                                        const void *src, size_t size, struct nsfslite_stride stride)
     *
     * @param handle the database handle
     * @param id the variable ID
     * @param start the starting byte offset
     * @param stop the ending byte offset (exclusive)
     * @param step the step size in bytes
     * @param data the data to write
     */
    public static void write(long handle, long id, long start, long stop, long step, byte[] data) {
        NsfsliteNative.write(handle, id, start, stop, step, data);
    }

    /**
     * Write data to a variable with step size of 1.
     *
     * @param handle the database handle
     * @param id the variable ID
     * @param start the starting byte offset
     * @param stop the ending byte offset (exclusive)
     * @param data the data to write
     */
    public static void write(long handle, long id, long start, long stop, byte[] data) {
        write(handle, id, start, stop, 1, data);
    }

    /**
     * Write data to a variable using a slice pattern [start:stop:step] within a transaction.
     *
     * @param handle the database handle
     * @param id the variable ID
     * @param txn the transaction handle
     * @param start the starting byte offset
     * @param stop the ending byte offset (exclusive)
     * @param step the step size in bytes
     * @param data the data to write
     */
    public static void write(long handle, long id, long txn, long start, long stop, long step, byte[] data) {
        NsfsliteNative.writeTx(handle, id, txn, start, stop, step, data);
    }

    /**
     * Write data to a variable with step size of 1 within a transaction.
     *
     * @param handle the database handle
     * @param id the variable ID
     * @param txn the transaction handle
     * @param start the starting byte offset
     * @param stop the ending byte offset (exclusive)
     * @param data the data to write
     */
    public static void write(long handle, long id, long txn, long start, long stop, byte[] data) {
        write(handle, id, txn, start, stop, 1, data);
    }

    /**
     * Remove data from a variable using a slice pattern [start:stop:step].
     * Equivalent to: ssize_t nsfslite_remove(nsfslite *n, uint64_t id, nsfslite_txn *tx,
     *                                         void *dest, size_t size, struct nsfslite_stride stride)
     *
     * @param handle the database handle
     * @param id the variable ID
     * @param start the starting byte offset
     * @param stop the ending byte offset (exclusive)
     * @param step the step size in bytes
     */
    public static void remove(long handle, long id, long start, long stop, long step) {
        NsfsliteNative.remove(handle, id, start, stop, step, false);
    }

    /**
     * Remove data from a variable with step size of 1.
     *
     * @param handle the database handle
     * @param id the variable ID
     * @param start the starting byte offset
     * @param stop the ending byte offset (exclusive)
     */
    public static void remove(long handle, long id, long start, long stop) {
        remove(handle, id, start, stop, 1);
    }

    /**
     * Remove and return data from a variable using a slice pattern [start:stop:step].
     *
     * @param handle the database handle
     * @param id the variable ID
     * @param start the starting byte offset
     * @param stop the ending byte offset (exclusive)
     * @param step the step size in bytes
     * @return the removed data as a byte array
     */
    public static byte[] removeAndReturn(long handle, long id, long start, long stop, long step) {
        return NsfsliteNative.remove(handle, id, start, stop, step, true);
    }

    /**
     * Remove and return data from a variable with step size of 1.
     *
     * @param handle the database handle
     * @param id the variable ID
     * @param start the starting byte offset
     * @param stop the ending byte offset (exclusive)
     * @return the removed data as a byte array
     */
    public static byte[] removeAndReturn(long handle, long id, long start, long stop) {
        return removeAndReturn(handle, id, start, stop, 1);
    }

    /**
     * Remove data from a variable using a slice pattern [start:stop:step] within a transaction.
     *
     * @param handle the database handle
     * @param id the variable ID
     * @param txn the transaction handle
     * @param start the starting byte offset
     * @param stop the ending byte offset (exclusive)
     * @param step the step size in bytes
     */
    public static void remove(long handle, long id, long txn, long start, long stop, long step) {
        NsfsliteNative.removeTx(handle, id, txn, start, stop, step, false);
    }

    /**
     * Remove data from a variable with step size of 1 within a transaction.
     *
     * @param handle the database handle
     * @param id the variable ID
     * @param txn the transaction handle
     * @param start the starting byte offset
     * @param stop the ending byte offset (exclusive)
     */
    public static void remove(long handle, long id, long txn, long start, long stop) {
        remove(handle, id, txn, start, stop, 1);
    }

    /**
     * Remove and return data from a variable using a slice pattern [start:stop:step] within a transaction.
     *
     * @param handle the database handle
     * @param id the variable ID
     * @param txn the transaction handle
     * @param start the starting byte offset
     * @param stop the ending byte offset (exclusive)
     * @param step the step size in bytes
     * @return the removed data as a byte array
     */
    public static byte[] removeAndReturn(long handle, long id, long txn, long start, long stop, long step) {
        return NsfsliteNative.removeTx(handle, id, txn, start, stop, step, true);
    }

    /**
     * Remove and return data from a variable with step size of 1 within a transaction.
     *
     * @param handle the database handle
     * @param id the variable ID
     * @param txn the transaction handle
     * @param start the starting byte offset
     * @param stop the ending byte offset (exclusive)
     * @return the removed data as a byte array
     */
    public static byte[] removeAndReturn(long handle, long id, long txn, long start, long stop) {
        return removeAndReturn(handle, id, txn, start, stop, 1);
    }

    ////////////////////////////////////////////
    // Transaction Management

    /**
     * Begin a new transaction.
     * Equivalent to: nsfslite_txn *nsfslite_begin_txn(nsfslite *n)
     *
     * @param handle the database handle
     * @return the transaction handle
     */
    public static long beginTxn(long handle) {
        return NsfsliteNative.beginTransaction(handle);
    }

    /**
     * Commit a transaction.
     * Equivalent to: int nsfslite_commit(nsfslite *n, nsfslite_txn *tx)
     *
     * @param handle the database handle
     * @param txn the transaction handle
     */
    public static void commit(long handle, long txn) {
        NsfsliteNative.commitTransaction(handle, txn);
    }
}
