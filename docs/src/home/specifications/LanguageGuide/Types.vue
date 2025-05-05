<script setup>

import Dropdown from "@/home/specifications/LanguageGuide/DropDown.vue";
import Primitive from "@/home/specifications/LanguageGuide/types/primitive.vue";
import Struct from "@/home/specifications/LanguageGuide/types/struct.vue";
import Union from "@/home/specifications/LanguageGuide/types/union.vue";
import Enum from "@/home/specifications/LanguageGuide/types/enum.vue";
import Varray from "@/home/specifications/LanguageGuide/types/varray.vue";
import Sarray from "@/home/specifications/LanguageGuide/types/sarray.vue";
import Examples from "@/home/specifications/LanguageGuide/types/examples.vue";
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