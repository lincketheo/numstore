<script setup lang="ts">
import ChangelogEntry from "@/home/development/changelog/ChangelogEntry.vue";
</script>

<template>
  <div class="changelog">
    <h1>Changelog</h1>

    <ChangelogEntry version="0.3.2" date="2025-05-01">
      <template #summary>
        Parser now handles all complex types correctly.
      </template>

      <template #added>
        <li>Support for nested struct/union parsing</li>
        <li>Recursive type resolution</li>
      </template>

      <template #changed>
        <li>Improved error messages in parser stage</li>
      </template>
    </ChangelogEntry>

    <ChangelogEntry version="0.3.1" date="2025-04-30">
      <template #summary>
        General AST enhancements and new query builder.
      </template>

      <template #added>
        <li>QueryBuilder abstraction</li>
      </template>

      <template #changed>
        <li>Generic AST node interface</li>
      </template>
    </ChangelogEntry>
  </div>
</template>

<style scoped>
.changelog {
  font-family: sans-serif;
  max-width: 800px;
  margin: 2rem auto;
  padding: 0 1rem;
}

</style>