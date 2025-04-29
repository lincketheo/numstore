<script setup>

import CommandDropdown from "@/commands/CommandDropdown.vue";
import Primitive from "@/commands/types/primitive.vue";
import Struct from "@/commands/types/struct.vue";
import Union from "@/commands/types/union.vue";
import Enum from "@/commands/types/enum.vue";
import Varray from "@/commands/types/varray.vue";
import Sarray from "@/commands/types/sarray.vue";
import Examples from "@/commands/types/examples.vue";
</script>

<template>
  <CommandDropdown>
    <template #title>
      TYPE
    </template>
    <pre v-pre>
     <code>
       TYPE := PRIM | STRUCT | UNION | ENUM | VARRAY | SARRAY
     </code></pre>
    <CommandDropdown>
      <template #title>
        PRIM: Primitive
      </template>
      <primitive/>
    </CommandDropdown>

    <CommandDropdown>
      <template #title>
        STRUCT: Struct
      </template>
      <struct/>
    </CommandDropdown>

    <CommandDropdown>
      <template #title>
        UNION: Union
      </template>
      <union/>
    </CommandDropdown>
    <CommandDropdown>
      <template #title>
        ENUM: Enum
      </template>
      <enum/>
    </CommandDropdown>
    <CommandDropdown>
      <template #title>
        VARRAY: Variable Length Array
      </template>
      <varray/>
    </CommandDropdown>
    <CommandDropdown>
      <template #title>
        SARRAY: Strict Array
      </template>
      <sarray/>
    </CommandDropdown>
    <CommandDropdown>
      <template #title>
        Examples
      </template>
      <examples/>
    </CommandDropdown>
  </CommandDropdown>

</template>

<style scoped>

</style>