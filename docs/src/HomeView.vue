<script setup>
</script>

<template>
  <nav id="left-nav">
    <div class="content-subtitle"><b>Overview</b></div>
    <hr>
    <ul>
      <li><RouterLink to="/overviewAbout">About Numstore</RouterLink></li>
      <li><RouterLink to="/overviewQuickStart">Quick Start</RouterLink></li>
    </ul>
    <div class="content-subtitle"><b>Specifications</b></div>
    <hr>
    <ul>
      <li><RouterLink to="/specificationsLanguageGuide">Language Guide</RouterLink></li>
      <li><RouterLink to="/specificationsConfiguring">Configuring</RouterLink></li>
    </ul>
    <div class="content-subtitle"><b>Development</b></div>
    <hr>
    <ul>
      <li><RouterLink to="/developmentChangelog">Changelog</RouterLink></li>
      <li><RouterLink to="/developmentContributing">Contributing</RouterLink></li>
    </ul>
  </nav>

  <div id="main-content">
    <RouterView/>
  </div>
</template>

<style scoped>
#left-nav {
  width: 18rem;
  padding: 20px 20px 20px 20px;
  background: var(--color-surface);
  border-right: 1px solid var(--color-primary);
}

/* Everything inside left nav get's on-surface */
#left-nav * {
  color: var(--color-on-surface);
}

#left-nav ul {
  list-style-type: none;
  padding: 1rem 0;
}

#main-content {
  background-color: var(--color-background);
  color: var(--color-on-background);
  flex: 1 1 auto;
  overflow: auto;
  padding-left: 0;
  padding-right: 0;
}
</style>