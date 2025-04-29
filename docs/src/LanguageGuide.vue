<script setup>
import {ref} from "vue";
import CommandsMain from "@/commands/CommandsMain.vue";

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
    <CommandsMain/>
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
  color: #6200ee;
  margin-bottom: 0.5rem;
}

.version {
  display: inline-block;
  background: #03dac6;
  color: #000;
  padding: 0.25rem 0.5rem;
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

/* Section Cards */
main section {
  background: #fff;
  padding: 1.5rem;
  border-radius: 8px;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
  margin-bottom: 2rem;
}

main section h2 {
  color: #6200ee;
  margin-bottom: 1rem;
  border-bottom: 2px solid #eee;
  padding-bottom: 0.5rem;
}

main section h3 {
  margin-top: 1rem;
  color: #333;
}

/* Code Blocks */
pre, code {
  background: #f5f5f5;
  font-family: 'Courier New', Courier, monospace;
  font-size: 0.95rem;
}

pre {
  padding: 1rem;
  border-radius: 4px;
  overflow: auto;
  margin: 1rem 0;
}

code {
  padding: 0.2rem 0.4rem;
  border-radius: 4px;
}

/* Lists */
ul, ol {
  margin-left: 1.5rem;
  margin-top: 0.5rem;
}

li {
  margin-bottom: 0.5rem;
}

/* Horizontal Rule */
hr {
  border: none;
  border-top: 1px solid #eee;
  margin: 2rem 0;
}

.dropdown {
  width: 300px;
  border: 1px solid #ddd;
  border-radius: 4px;
  overflow: hidden;
}

.dropdown-label {
  display: flex;
  justify-content: space-between;
  padding: 0.75em 1em;
  background: #f5f5f5;
  cursor: pointer;
  user-select: none;
}

.dropdown-label:hover {
  background: #eaeaea;
}

.dropdown-content {
  padding: 1em;
  background: white;
  border-top: 1px solid #ddd;
}

/* Simple slide+fade transition */
.slide-fade-enter-active,
.slide-fade-leave-active {
  transition: all 0.3s ease;
}

.slide-fade-enter-from,
.slide-fade-leave-to {
  transform: translateY(-5px);
  opacity: 0;
}
</style>