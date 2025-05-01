<script setup>
import Dropdown from "@/commands/DropDown.vue";
</script>

<template>
  <Dropdown>
    <template #title>
      delete
    </template>
    <h4>Request:</h4>
    <pre v-pre>
      <code>
delete &ltVNAME&gt &ltRANGE_STR&gt?
      </code></pre>
    <br>
    <h6>Examples</h6>
    <pre v-pre><code>
delete a

delete a[0:1:10]

delete a[::]

delete a[::2]

delete a[0]

delete a[:10]
    </code></pre>
  </Dropdown>
</template>
