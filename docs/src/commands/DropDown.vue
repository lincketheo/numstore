<template>
  <div class="dropdown">
    <!-- Title slot you click -->
    <div class="dropdown-label" @click="open = !open">
      <slot name="title">
        <!-- Fallback title -->
        Click to expand
      </slot>
      <span class="icon">{{ open ? '▾' : '▸' }}</span>
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
  border: 1px solid #ddd;
  border-radius: 4px;
  overflow: hidden;
}

.dropdown-label {
  display: flex;
  justify-content: space-between;
  align-items: center;
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
