<script setup>
</script>

<template>
  <div id="layout">

    <nav>
      <div id="logo">
        <img id="logo" alt="Logo" src="./assets/logo.png">
        <span>Numstore</span>
      </div>

      <div id="pages">
        <RouterLink to="/">Home</RouterLink>
        <RouterLink to="/about">About</RouterLink>
        <RouterLink to="/news">News</RouterLink>
      </div>
    </nav>

    <main id="content">
      <RouterView/>
    </main>

    <footer>
      &copy; 2025 Lincke Solutions LLC
    </footer>
  </div>

</template>

<style scoped>
/*
Content layout
  HEADER
  CONTENT
  FOOTER
Note BODY has it's own internal layout
 */
#layout {
  display: flex;
  flex-direction: column;
  overflow: hidden;
  height: 100vh;
}

/*
Row layout
LOGO [ ------ ] PAGES
*/
nav {
  display: flex;
  align-items: center;
  justify-content: space-between;
  background: var(--color-primary);
  padding: 1rem 1rem;
}

/* Everything inside nav gets on-primary */
nav * {
  color: var(--color-on-primary);
}

/* Right most pages get padding in between each other */
nav #pages a {
  padding: 1rem;
}

nav #pages a:hover {
  background-color: var(--color-secondary);
  color: var(--color-on-secondary);
}

#logo {
  display: flex;
  align-items: center;
}

#logo img {
  height: 32px;
  margin-right: 0.5rem;
}

/*
Row layout
LEFTNAV MAINCONTENT
 */
main {
  display: flex;
  overflow-x: auto;
  overflow-y: auto;
  flex: 1 1 auto;
}

footer {
  background-color: var(--color-surface);
  color: var(--color-on-surface);
  border-top: none;
  width: 100%;
  z-index: 1;
}
</style>

