<script setup>
import Dropdown from "@/home/specifications/LanguageGuide/DropDown.vue";
</script>

<template>
  <Dropdown>
    <template #title>
      delete
    </template>
    <template #subtitle>
      Plain Text Json (user friendly) version of the delete operation
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
