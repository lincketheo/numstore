<template>
  <div class="dropdown">
    <!-- Title slot you click -->
    <div class="dropdown-label" @click="open = !open">
      <div>
        <h4 id="title">
          <slot name="title">
            <!-- Fallback title -->
            Click to expand
          </slot>
        </h4>
        <p id="subtitle">
          <em>
          <slot name="subtitle">
          </slot>
          </em>
        </p>
      </div>
      <span>{{ open ? '▾' : '▸' }}</span>
    </div>

    <!-- Content slot, revealed when `open` is true -->
    <transition name="slide-fade">
      <div v-if="open" class="dropdown-content">
        <slot>
          <!-- Default content if none is provided -->
          <p>This is the revealed content!</p>
        </slot>
      </div>
    </transition>
  </div>
</template>

<script>
export default {
  name: 'DropdownSection',
  data() {
    return {
      open: false
    };
  }
};
</script>

<style scoped>
.dropdown {
  width: 100%; /* fill parent width */
  border-radius: 4px;
  overflow: hidden;
}

.dropdown-label {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0.75em 1em;
  background: var(--color-surface);
  cursor: pointer;
  user-select: none;
}

.dropdown-label:hover {
  background: var(--color-primary);
}

#subtitle {
  display: none;
}

.dropdown-label:hover #subtitle {
  display: block;
}

.dropdown-content {
  padding: 1em;
  background: var(--color-background);
}
</style>
