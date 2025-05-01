<script setup>

import Dropdown from "@/commands/DropDown.vue";
import Primitive from "@/commands/types/primitive.vue";
import Struct from "@/commands/types/struct.vue";
import Union from "@/commands/types/union.vue";
import Enum from "@/commands/types/enum.vue";
import Varray from "@/commands/types/varray.vue";
import Sarray from "@/commands/types/sarray.vue";
import Examples from "@/commands/types/examples.vue";
</script>

<template>
  <Dropdown>
    <template #title>
      TYPE
    </template>
    <pre v-pre>
     <code>
       TYPE := PRIM | STRUCT | UNION | ENUM | VARRAY | SARRAY
     </code></pre>
    <Dropdown>
      <template #title>
        PRIM: Primitive
      </template>
      <primitive/>
    </Dropdown>

    <Dropdown>
      <template #title>
        STRUCT: Struct
      </template>
      <struct/>
    </Dropdown>

    <Dropdown>
      <template #title>
        UNION: Union
      </template>
      <union/>
    </Dropdown>
    <Dropdown>
      <template #title>
        ENUM: Enum
      </template>
      <enum/>
    </Dropdown>
    <Dropdown>
      <template #title>
        VARRAY: Variable Length Array
      </template>
      <varray/>
    </Dropdown>
    <Dropdown>
      <template #title>
        SARRAY: Strict Array
      </template>
      <sarray/>
    </Dropdown>
    <Dropdown>
      <template #title>
        Examples
      </template>
      <examples/>
    </Dropdown>
  </Dropdown>

</template>

<style scoped>

</style>