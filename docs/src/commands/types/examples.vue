<script setup>

</script>

<template>
<pre v-pre><code>
U8
F128
CF256
CI128
CU32
BOOL
BIT
struct { x U16, y F32 }
union { a I32, b F64, flag BOOL }
enum { RED, GREEN, BLUE, YELLOW }
[]U8
[][]F32
[16]U8
[8][8]U16
struct { count U32, data []U8 }
union { code I16, buffer [64]U8 }
enum { SUCCESS, ERROR, PENDING, TIMEOUT }
[]struct { id U64, coords [3]F64 }
[10][]I32
struct { u union { a U8, b U16 }, arr [4]F32 }
[]union { a U8, b I8 }
[2][3]struct { x U8, y U8 }
</code></pre>
</template>

<style scoped>

</style>