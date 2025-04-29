<script setup>

</script>

<template>
<pre v-pre>
  <code>
  PRIM :=
      U8      # An 8-bit unsigned integer (aka a byte)
      U16     # A 16-bit unsigned integer
      U32     # A 32-bit unsigned integer
      U64     # A 64-bit unsigned integer

      I8      # An 8-bit signed integer
      I16     # A 16-bit signed integer
      I32     # A 32-bit signed integer
      I64     # A 64-bit signed integer

      F16     # A 16-bit floating-point number (half precision)
      F32     # A 32-bit floating-point number (single precision)
      F64     # A 64-bit floating-point number (double precision)
      F128    # A 128-bit floating-point number (quad precision)

      CF32    # A complex number of two F16 components (32 bits total)
      CF64    # A complex number of two F32 components (64 bits total)
      CF128   # A complex number of two F64 components (128 bits total)
      CF256   # A complex number of two F128 components (256 bits total)

      CI16    # A complex integer of two I8 components (32 bits total)
      CI32    # A complex integer of two I16 components (64 bits total)
      CI64    # A complex integer of two I32 components (128 bits total)
      CI128   # A complex integer of two I64 components (256 bits total)

      CU16    # A complex unsigned integer of two U8 components (16 bits total)
      CU32    # A complex unsigned integer of two U16 components (32 bits total)
      CU64    # A complex unsigned integer of two U32 components (64 bits total)
      CU128   # A complex unsigned integer of two U64 components (128 bits total)

      BOOL    # A boolean value - true or false (8 bits total)
      BIT     # A single bit flag (1 bit total)
</code></pre>
</template>

<style scoped>

</style>