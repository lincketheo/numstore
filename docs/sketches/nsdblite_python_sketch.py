
with nsdblite.open("test.db", "test.wal") as conn:
    # Create a variable (returns the variable)
    tv = conn.create(
            name = "test_variable", 
            dtype = """
                    struct {
                        a u8,
                        b union {
                            c i64,
                            d f16,
                        },
                        c [10]int,
                    }
                    """
            )

    # Same variable as above, just another way of getting it
    tv = conn.get(name = "test_variable")

    # Length 
    print(len(tv))

    # INSERT
    tv.insert(
            offset = 0,
            data = np.array([(1, 1, 1.3), (2, 3, 4.5)])
    )

    # READ
    arr = tv[0:10:20] 

    # WRITE
    tv[0:10:20] = np.array([(1, 1, 1.3), (2, 3, 4.5)])

    # REMOVE
    del tv[0:10:20] 
    # OR
    removed = tv.remove()[0:10:20] # Returns values deleted

    # Delete a variable
    del tv
    # OR
    conn.delete(name = "test_variable")

