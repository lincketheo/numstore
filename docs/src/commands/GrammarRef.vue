<script setup>

import Dropdown from "@/commands/DropDown.vue";

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
import Commands from "@/commands/commands.vue";
import Others from "@/commands/others.vue";
</script>

<template>
  <h3>Type Reference</h3>
  <type/>
  <br>
  <h3>Available Operation Commands</h3>
  <commands/>
  <br>
  <h3>Other</h3>
  <others/>

</template>

<style scoped>

</style>