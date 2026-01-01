package com.numstore.nsfslite;

/**
 * Stride parameters for read/write/remove operations.
 * Maps to the C struct nsfslite_stride.
 */
public class Stride {
    /** Starting byte offset */
    public final long bstart;
    /** Stride between elements in bytes */
    public final long stride;
    /** Number of elements */
    public final long nelems;

    /**
     * Create a new Stride.
     *
     * @param bstart starting byte offset
     * @param stride stride between elements in bytes
     * @param nelems number of elements
     */
    public Stride(long bstart, long stride, long nelems) {
        this.bstart = bstart;
        this.stride = stride;
        this.nelems = nelems;
    }

    @Override
    public String toString() {
        return "Stride{bstart=" + bstart + ", stride=" + stride + ", nelems=" + nelems + "}";
    }
}
