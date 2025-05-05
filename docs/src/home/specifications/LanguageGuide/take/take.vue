<script setup>
import Dropdown from "@/home/specifications/LanguageGuide/DropDown.vue";
</script>

<template>
  <Dropdown>
    <template #title>
      take
    </template>
    <h4>Request:</h4>
    <br>
    <h6>Take elements from a single variable:</h6>
    <pre v-pre><code>take &ltVNAME&gt &ltRANGE_STR&gt</code></pre>
    <br>
    <h6>Take linked elements from multiple variables:</h6>
    <pre v-pre><code>take [&ltVNAME&gt (,&ltVNAME&gt)*]&ltRANGE_STR&gt</code></pre>
    <br>
    <h6>Examples</h6>
    <pre v-pre><code>
take a[10]
take [a, b][10]
take [a, b, c][10:100]
take [a, b, c][10:100:2]

</code></pre>
  </Dropdown>
</template>
