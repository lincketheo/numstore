<script setup>
import Dropdown from "@/commands/DropDown.vue";
</script>

<template>
    <Dropdown>
      <template #title>
        read
      </template>
      <h4>Request:</h4>
      <br>
      <h6>Read elements from a single variable:</h6>
      <pre v-pre><code>read &ltVNAME&gt &ltRANGE_STR&gt</code></pre>
      <br>
      <h6>Read linked elements from multiple variables:</h6>
      <pre v-pre><code>read [&ltVNAME&gt (,&ltVNAME&gt)*]&ltRANGE_STR&gt</code></pre>
      <br>
      <h6>Examples</h6>
      <pre v-pre><code>
read a[10]
read [a, b][10]
read [a, b, c][10:100]
read [a, b, c][10:100:2]

</code></pre>
  </Dropdown>
</template>
