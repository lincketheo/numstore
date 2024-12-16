//
// Created by theo on 12/15/24.
//

#include "numstore/utils/files.hpp"

#include <numstore/facades/posix.hpp>

#include "numstore/macros.hpp"
#include "numstore/errors.hpp"
#include "numstore/facades/stdlib.hpp"
#include "numstore/testing.hpp"


extern "C" {
#include <sys/stat.h>
PRIVATE ns_ret_t
fsize_seekable(size_t *dest, FILE *fp) {
    ASSERT(fp);
    size_t start, bytes;

    // Get the starting position
    ns_ret_t_fallthrough(ftell_f (&start, fp));

    // Seek to the end
    ns_ret_t_fallthrough(fseek_f (fp, 0, SEEK_END));

    // Get the current offset
    ns_ret_t_fallthrough(ftell_f (&bytes, fp));

    // Reset back to the start
    ns_ret_t_fallthrough(fseek_f (fp, start, SEEK_SET));

    *dest = bytes;

    return NS_OK;
}

PRIVATE ns_ret_t
fsize_non_seekable(size_t *dest, FILE *fp) {
    ASSERT(fp);
    size_t bytes = 0;

    do {
        size_t read = 1024;
        ns_ret_t_fallthrough(fread_discard (fp, 1, &read));
        bytes += read;
    } while (!feof(fp));

    *dest = bytes;

    return NS_OK;
}

ns_ret_t
fsize(size_t *dest, FILE *fp, const size_t size) {
    size_t blen;

    bool _is_seekable;
    ns_ret_t_fallthrough(is_seekable (&_is_seekable, fp));

    if (_is_seekable) {
        ns_ret_t_fallthrough(fsize_seekable (&blen, fp));
    } else {
        ns_ret_t_fallthrough(fsize_non_seekable (&blen, fp));
    }

    // Size should be a multiple of size
    if (blen % size != 0) {
        ns_log(WARN, "Expected file size in bytes to be a "
               "multiple of: %zu, but size was: %zu\n");
        return NS_EINVAL;
    }

    *dest = blen / size;

    return NS_OK;
}

TEST(fsize_or_abort) {
    FILE *fp = fopen("tmpfile", "wb");

    uint8_t data[] = {1, 2, 3, 4, 5};
    fwrite(data, 1, 5, fp);

    size_t flen;
    tjl_ret_unwrap(fsize(&flen, fp, 1));
    test_assert_equal(flen, 5lu, "%lu");

    fclose(fp);
    remove("tmpfile");
}

ns_ret_t fread_malloc(bytes *dest, FILE *fp, size_t size, size_t len) {
    ASSERT(dest);
    ASSERT(fp);
    ASSERT(size > 0);
    ASSERT(len > 0);

    ns_ret_t ret;
    void *ptr;
    size_t _len = len;

    // Allocate enough memory
    ns_ret_t_onfail(malloc_f(&ptr, len * size), cleanup);

    // Read data
    ns_ret_t_onfail(fread_f(ptr, size, &_len, fp), cleanup);

    // Read was less - reallocate
    if (_len != len) {
        ns_ret_t_onfail(realloc_f(&ptr, _len * size), cleanup);
    }

    dest->data = (byte *) ptr;
    dest->len = _len;

    return ret;

cleanup:
    if (ptr)
        free_f(ptr);
    return ret;
}

TEST(fread_malloc) {
    FILE *fp = fopen("tmpfile", "wb");
    uint8_t data[] = {1, 2, 3, 4, 5};
    fwrite(data, 1, 5, fp);
    fclose(fp);
    fp = fopen("tmpfile", "rb");

    bytes ret;
    fread_malloc(&ret, fp, 1, 5);
    test_assert_equal(ret.len, 5, "%d");
    uint8_t *_data = ret.data;
    test_assert_equal(_data[0], 1, "%d");
    test_assert_equal(_data[1], 2, "%d");
    test_assert_equal(_data[2], 3, "%d");
    test_assert_equal(_data[3], 4, "%d");
    test_assert_equal(_data[4], 5, "%d");
    free_f(ret.data);

    rewind(fp);
    fread_malloc(&ret, fp, 1, 10);
    test_assert_equal(ret.len, 5, "%d");
    _data = ret.data;
    test_assert_equal(_data[0], 1, "%d");
    test_assert_equal(_data[1], 2, "%d");
    test_assert_equal(_data[2], 3, "%d");
    test_assert_equal(_data[3], 4, "%d");
    test_assert_equal(_data[4], 5, "%d");
    free_f(ret.data);

    remove("tmpfile");
}

ns_ret_t fread_all_malloc_noseek(bytes *dest, FILE *fp, size_t size, size_t limit);

ns_ret_t fread_all_mmap_seek(bytes *dest, FILE *fp, size_t size, int prot, int flags);

ns_ret_t fread_discard(FILE *file, size_t size, size_t *n) {
    ASSERT(size <= 1024);
    ASSERT(size > 0);

    char buffer[1024];
    const size_t bn = sizeof (buffer) / size;
    size_t _n = *n;

    while (_n > 0) {
        const size_t next = _n < bn ? _n : bn;
        size_t read = next;

        ns_ret_t_fallthrough(fread_f(buffer, size, &read, file));

        _n -= read;
        if (read != next) {
            if (feof(file)) {
                break;
            }
            unreachable();
        }

        ASSERT(*n >= read);
    }

    ASSERT(*n >= _n);

    *n = *n - _n;

    return NS_OK;
}

TEST(fread_discard) {
    FILE *fp = fopen("tmpfile", "wb");
    const uint8_t data[] = {1, 2, 3, 4, 5};
    fwrite(data, 1, sizeof(data), fp);
    fclose(fp);

    fp = fopen("tmpfile", "rb");

    size_t bytes_to_discard = 3;
    size_t original_bytes = bytes_to_discard;
    ns_ret_t status = fread_discard(fp, 1, &bytes_to_discard);
    test_assert_equal(status, NS_OK, "%d");
    test_assert_equal(bytes_to_discard, original_bytes, "%d");
    test_assert_equal(ftell(fp), 3, "%ld");

    bytes_to_discard = 5;
    original_bytes = bytes_to_discard;
    status = fread_discard(fp, 1, &bytes_to_discard);
    test_assert_equal(status, NS_OK, "%d");
    test_assert_equal(bytes_to_discard, 2, "%d");
    test_assert_equal(ftell(fp), 5, "%ld");

    test_assert_equal(feof(fp), 1, "%d");

    fclose(fp);
    remove("tmpfile");
}

ns_ret_t is_seekable(bool *dest, FILE *fp) {
    ASSERT(dest);
    ASSERT(fp);

    const ns_ret_t ret = fseek_f(fp, 0, SEEK_SET);
    if (ret == NS_PIPE) {
        *dest = false;
    }
    if (ret != NS_OK) {
        return ret;
    }
    *dest = true;
    return NS_OK;
}

TEST(is_seekable) {
    bool is_seekable_result;

    FILE *seekable_file = fopen("seekable_tmpfile", "wb+");
    uint8_t data[] = {1, 2, 3, 4, 5};
    fwrite(data, 1, sizeof(data), seekable_file);
    rewind(seekable_file);

    ns_ret_t ret = is_seekable(&is_seekable_result, seekable_file);
    test_assert_equal(ret, NS_OK, "%d");
    test_assert_equal(is_seekable_result, 1, "%d");

    fclose(seekable_file);
    remove("seekable_tmpfile");

    FILE *pipe_file = popen("echo 'test'", "r");
    ret = is_seekable(&is_seekable_result, pipe_file);
    test_assert_equal(ret, NS_PIPE, "%d");
    test_assert_equal(is_seekable_result, 0, "%d");
    pclose(pipe_file);

    remove("seekable_tmpfile2");
}

ns_ret_t fcopy(FILE *ifp, FILE *ofp) {
    ASSERT(ifp);
    ASSERT(ofp);

    do {
        byte buffer[1024];
        size_t read = 1024;
        ns_ret_t_fallthrough(fread_f(buffer, 1, &read, ifp));
        if (read > 0) {
            ns_ret_t_fallthrough(fwrite_f(buffer, 1, read, ofp));
        }
    } while (!feof(ifp));
}

TEST(fcopy) {
    // Create a temporary input file and write test data
    FILE *input_file = fopen("input_tmpfile", "wb");
    uint8_t data[] = {1, 2, 3, 4, 5};
    fwrite(data, 1, sizeof(data), input_file);
    fclose(input_file);

    // Open the input file for reading and create an output file
    input_file = fopen("input_tmpfile", "rb");
    FILE *output_file = fopen("output_tmpfile", "wb");

    // Perform the copy
    ns_ret_t ret = fcopy(input_file, output_file);
    test_assert_equal(ret, NS_OK, "%d");

    fclose(input_file);
    fclose(output_file);

    // Verify the contents of the output file
    output_file = fopen("output_tmpfile", "rb");
    uint8_t buffer[5];
    fread(buffer, 1, sizeof(buffer), output_file);

    test_assert_equal(buffer[0], 1, "%d");
    test_assert_equal(buffer[1], 2, "%d");
    test_assert_equal(buffer[2], 3, "%d");
    test_assert_equal(buffer[3], 4, "%d");
    test_assert_equal(buffer[4], 5, "%d");

    fclose(output_file);

    // Cleanup
    remove("input_tmpfile");
    remove("output_tmpfile");
}


ns_ret_t fread_zero_top(void *ptr, size_t size, size_t *n, FILE *fp);

ns_ret_t fread_shift_half(void *ptr, size_t size, size_t *n, FILE *fp);

ns_ret_t fread_shift_half_zero_top(void *ptr, size_t size, size_t *n, FILE *fp);

ns_ret_t mmap_create_wrb(bytes *dest, const char *fname, size_t bytes);

ns_ret_t mmap_create_wrb_wbase(bytes *dest, const char *base, const char *fname, size_t size, size_t len);

ns_ret_t is_valid_folder(bool *dest, const char *fname) {
    ASSERT(dest);
    ASSERT(fname);

    struct stat sb{};
    const ns_ret_t ret = stat_f(fname, &sb);

    if (ret == NS_OK) {
        *dest = S_ISDIR(sb.st_mode);
        return NS_OK;
    }

    switch (ret) {
        case NS_NOTFOUND:
        case NS_NOTDIR:
        case NS_NOENT:
        case NS_BADF: {
            *dest = false;
            return NS_OK;
        }
        default:
            return ret;
    }
}
}
