<script setup>
import GrammarRef from "@/home/specifications/LanguageGuide/GrammarRef.vue";
import {ref} from "vue";

const label = ref("Click to expand")
const open = ref(false)
</script>

<template>
  <header>
    <h1>Numstore Language Guide</h1>
    <span class="version">VERSION 0.0.1-WIP</span>
    <p>These docs are for feature posterity only. v0.0.1 is still a work in progress.</p>
    <p>Numstore is a compact, expressive language for recording contiguous “memory‑shaped” array data.</p>
  </header>

  <main>
    <GrammarRef/>
  </main>
</template>

<style scoped>
/* Header */
header {
  text-align: center;
  margin-bottom: 2rem;
}

header h1 {
  font-size: 2.5rem;
  margin: 0.5rem 0.5rem;
}

.version {
  display: inline-block;
  background: var(--color-primary);
  color: var(--color-on-primary);
  padding: 0.5rem 0.5rem;
  border-radius: 4px;
  font-size: 0.9rem;
  vertical-align: middle;
}

header p {
  margin-top: 1rem;
  max-width: 800px;
  margin-left: auto;
  margin-right: auto;
  font-size: 1rem;
}
</style>