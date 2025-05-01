<script setup>

import Binsert from "@/commands/insert/binsert.vue";
import Bdelete from "@/commands/delete/bdelete.vue";
import Btake from "@/commands/take/btake.vue";
import Insert from "@/commands/insert/insert.vue";
import Bcreate from "@/commands/create/bcreate.vue";
import Bappend from "@/commands/append/bappend.vue";
import Bupdate from "@/commands/update/bupdate.vue";
import Append from "@/commands/append/append.vue";
import Dropdown from "@/commands/DropDown.vue";
import Update from "@/commands/update/update.vue";
import Bread from "@/commands/read/bread.vue";
import Take from "@/commands/take/take.vue";
import Create from "@/commands/create/create.vue";
import Read from "@/commands/read/read.vue";
import Delete from "@/commands/delete/delete.vue";
</script>

<template>

  <Dropdown>
    <template #title>
      Create
    </template>
    <create/>
    <bcreate/>
  </Dropdown>

  <Dropdown>
    <template #title>
      Delete
    </template>
    <delete/>
    <bdelete/>
  </Dropdown>

  <Dropdown>
    <template #title>
      Append
    </template>
    <append/>
    <bappend/>
  </Dropdown>

  <Dropdown>
    <template #title>
      Insert
    </template>
    <insert/>
    <binsert/>
  </Dropdown>

  <Dropdown>
    <template #title>
      Update
    </template>
    <update/>
    <bupdate/>
  </Dropdown>

  <Dropdown>
    <template #title>
      Read
    </template>
    <read/>
    <bread/>
  </Dropdown>

  <Dropdown>
    <template #title>
      Take
    </template>
    <take/>
    <btake/>
  </Dropdown>

</template>

<style scoped>

</style>