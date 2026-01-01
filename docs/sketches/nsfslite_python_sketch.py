
with nsfslite.open("test.db", "test.wal") as conn:
    # Create a variable (returns the variable)
    tv = conn.create(name = "test_variable")

    # Same variable as above, just another way of getting it
    tv = conn.get(name = "test_variable")

    # Length 
    print(len(tv))

    # INSERT - data can be anything "bytes like" 
    # bytes, string, 
    # array (interpreted as bytes), numpy array (interpreted as bytes)
    # etc.
    tv.insert(offset = 0, data = "foo bar biz buz")

    # READ - data is bytes
    data = tv[0:10:20] 

    # WRITE - same thing - data can be anything
    tv[0:10:20] = np.array([(1, 1, 1.3), (2, 3, 4.5)])

    # REMOVE 
    del tv[0:10:20] 
    # OR - removed is bytes
    removed = tv.remove()[0:10:20] # Returns values deleted

    # Delete a variable
    del tv
    # OR
    conn.delete(name = "test_variable")

