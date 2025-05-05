<script setup>

import Type from "@/home/specifications/LanguageGuide/Types.vue";
import Commands from "@/home/specifications/LanguageGuide/Commands.vue";
import Others from "@/home/specifications/LanguageGuide/Others.vue";
</script>

<template>
  <section>
    <h3>Available Operation Commands</h3>
    <p><em>Things you can "do" to an array of data</em></p>
    <commands/>
  </section>

  <section>
    <h3>Type Reference</h3>
    <type/>
  </section>

  <section>
    <h3>Other</h3>
    <others/>
  </section>

</template>

<style scoped>
section {
  padding: 2rem;
}
h3 {
  padding: 0.25rem;
}
p {
  padding: 0.25rem;
}
</style>