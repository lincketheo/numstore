<script setup>

import Binsert from "@/home/specifications/LanguageGuide/insert/binsert.vue";
import Bdelete from "@/home/specifications/LanguageGuide/delete/bdelete.vue";
import Btake from "@/home/specifications/LanguageGuide/take/btake.vue";
import Insert from "@/home/specifications/LanguageGuide/insert/insert.vue";
import Bcreate from "@/home/specifications/LanguageGuide/create/bcreate.vue";
import Bappend from "@/home/specifications/LanguageGuide/append/bappend.vue";
import Bupdate from "@/home/specifications/LanguageGuide/update/bupdate.vue";
import Dropdown from "@/home/specifications/LanguageGuide/DropDown.vue";
import Update from "@/home/specifications/LanguageGuide/update/update.vue";
import Bread from "@/home/specifications/LanguageGuide/read/bread.vue";
import Take from "@/home/specifications/LanguageGuide/take/take.vue";
import Create from "@/home/specifications/LanguageGuide/create/create.vue";
import Read from "@/home/specifications/LanguageGuide/read/read.vue";
import Delete from "@/home/specifications/LanguageGuide/delete/delete.vue";
import Append from "@/home/specifications/LanguageGuide/append/append.vue";
</script>

<template>

  <Dropdown>
    <template #title>
      Create
    </template>
    <template #subtitle>
      Create a variable with a certain type
    </template>
    <create/>
    <bcreate/>
  </Dropdown>

  <Dropdown>
    <template #title>
      Delete
    </template>
    <template #subtitle>
      Delete a variable or delete one or more data elements starting at a certain index
    </template>
    <delete/>
    <bdelete/>
  </Dropdown>

  <Dropdown>
    <template #title>
      Append
    </template>
    <template #subtitle>
      Add one or more data elements to the end of an array
    </template>
    <append/>
    <bappend/>
  </Dropdown>

  <Dropdown>
    <template #title>
      Insert
    </template>
    <template #subtitle>
      Add one or more data elements at a specified location - pushing elements to the right
    </template>
    <insert/>
    <binsert/>
  </Dropdown>

  <Dropdown>
    <template #title>
      Update
    </template>
    <template #subtitle>
      Overwrite one or more data elements starting at a specific location
    </template>
    <update/>
    <bupdate/>
  </Dropdown>

  <Dropdown>
    <template #title>
      Read
    </template>
    <template #subtitle>
      Read one or more data elements starting at a certain index
    </template>
    <read/>
    <bread/>
  </Dropdown>

  <Dropdown>
    <template #title>
      Take
    </template>
    <template #subtitle>
      Read and delete one or more data elements starting at a certain index
    </template>
    <take/>
    <btake/>
  </Dropdown>

</template>

<style scoped>

</style>