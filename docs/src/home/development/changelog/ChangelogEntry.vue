<template>
  <section class="entry">
    <div class="entry-header">
      <h2>Version {{ version }}</h2>
      <span class="entry-date">{{ date }}</span>
    </div>

    <p v-if="$slots.summary" class="summary">
      <slot name="summary" />
    </p>

    <div v-if="$slots.added">
      <h3>Added</h3>
      <ul>
        <slot name="added" />
      </ul>
    </div>

    <div v-if="$slots.changed">
      <h3>Changed</h3>
      <ul>
        <slot name="changed" />
      </ul>
    </div>

    <div v-if="$slots.removed">
      <h3>Removed</h3>
      <ul>
        <slot name="removed" />
      </ul>
    </div>
  </section>
</template>

<script setup>
defineProps({
  version: {
    type: String,
    required: true
  },
  date: {
    type: String,
    required: true
  }
})
</script>

<style scoped>
.entry {
  margin-bottom: 2rem;
}

.entry-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.entry-header h2 {
  font-size: 1.2rem;
  margin: 0;
}

.entry-date {
  font-size: 0.9rem;
  color: var(--color-primary);
}

.summary {
  margin: 0.5rem 0 1rem;
}

h3 {
  margin-top: 1rem;
  margin-bottom: 0.3rem;
  font-size: 1rem;
}

ul {
  margin-left: 1.5rem;
}
</style>