<script setup>

import CommandDropdown from "@/commands/CommandDropdown.vue";

import Bappend from "@/commands/append/bappend.vue";
import Append from "@/commands/append/append.vue";
import Bcreate from "@/commands/create/bcreate.vue";
import Delete from "@/commands/delete/delete.vue";
import Bdelete from "@/commands/delete/bdelete.vue";
import Insert from "@/commands/insert/insert.vue";
import Binsert from "@/commands/insert/binsert.vue";
import Bread from "@/commands/read/bread.vue";
import Read from "@/commands/read/read.vue";
import Update from "@/commands/update/update.vue";
import Bupdate from "@/commands/update/bupdate.vue";
import Take from "@/commands/take/take.vue";
import Btake from "@/commands/take/btake.vue";
import Create from "@/commands/create/create.vue";
import Type from "@/commands/type.vue";
</script>

<template>
  <h3>Type Reference</h3>
  <type/>
  <br>
  <h3>Available Operation Commands</h3>
  <CommandDropdown>
    <template #title>
      Create
    </template>
    <create/>
    <bcreate/>
  </CommandDropdown>

  <CommandDropdown>
    <template #title>
      Delete
    </template>
    <delete/>
    <bdelete/>
  </CommandDropdown>

  <CommandDropdown>
    <template #title>
      Append
    </template>
    <append/>
    <bappend/>
  </CommandDropdown>

  <CommandDropdown>
    <template #title>
      Insert
    </template>
    <insert/>
    <binsert/>
  </CommandDropdown>

  <CommandDropdown>
    <template #title>
      Update
    </template>
    <update/>
    <bupdate/>
  </CommandDropdown>

  <CommandDropdown>
    <template #title>
      Read
    </template>
    <read/>
    <bread/>
  </CommandDropdown>

  <CommandDropdown>
    <template #title>
      Take
    </template>
    <take/>
    <btake/>
  </CommandDropdown>
</template>

<style scoped>

</style>