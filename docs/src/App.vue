<script setup>
import LanguageGuide from "@/LanguageGuide.vue";
</script>

<template>
  <div id="layout">

    <header>
      <nav id="top-nav">
        <div id="logo">
          <img id="logo" alt="Logo" src="./assets/logo.png">
        </div>
        <span>Numstore</span>
        <a href="/about">About</a>
        <a href="/news">News</a>
      </nav>
    </header>

    <main id="content">
      <nav id="left-nav">
        <ul>
          <li>
            <span>Overview</span>
            <ul>
              <li><a href="/about">About Numstore</a></li>
              <li><a href="/quick-start">Quick Start</a></li>
            </ul>
          </li>
        </ul>
      </nav>

      <div id="main-content">
        <LanguageGuide/>
      </div>
    </main>

    <footer id="footer">
      &copy; 2025 Lincke Solutions LLC
    </footer>
  </div>

</template>

<style scoped>
#layout {
  display: flex;
  flex-direction: column;
  overflow: hidden;
  height: 100vh;
}

#top-nav {
  display: flex;
  align-items: center;
  background: var(--color-primary);
  color: var(--color-on-primary);
  padding: 0 1rem;
  height: 56px;
  flex-shrink: 0;
}

#logo {
  display: flex;
  align-items: center;
}

#logo img {
  height: 32px;
  margin-right: 0.5rem;
}

#content {
  display: flex;
  overflow-x: auto;
  overflow-y: auto;
  flex: 1 1 auto;
}

#left-nav {
  width: 18rem;
  padding: 20px 20px 20px 26px;
  background: var(--color-background);
  overflow-y: scroll;
  overflow-x: auto;
  flex: 0 0 auto;
  border-right: 1px solid var(--color-primary);
}

#left-nav ul {
  list-style-type: none;
  margin: 0;
  padding: 0;
}

#left-nav ul li a {
  color: var(--color-on-background);
  display: block;
  font-size: .95rem;
  position: relative;
}

#main-content {
  flex: 1 1 auto;
  overflow: auto;
  padding-left: 0;
  padding-right: 0;
}

#footer {
  background-color: var(--color-background);
  border-top: none;
  width: 100%;
  z-index: 1;
}

.tabs {
  margin-left: auto;
  display: flex;
}

.tabs a {
  color: var(--color-on-primary);
  text-decoration: none;
  margin-left: 1rem;
  padding: 0.5rem;
}

.tabs a.active {
  border-bottom: 2px solid var(--color-secondary);
}

/* Left Sidebar Wrapper */
.left-wrapper {
  display: flex;
  flex-direction: column;
  width: 240px;
  background: var(--color-surface);
  border-right: 1px solid #ddd;
  flex-shrink: 0;
}

/* Sidebar Navigation (scrollable) */
.sidebar-nav {
  flex: 1;
  overflow-y: auto;
  padding: 1rem;
}

.sidebar-nav ul {
  list-style: none;
}

.sidebar-nav .heading {
  display: block;
  font-weight: bold;
  margin-top: 1rem;
}

.sidebar-nav ul ul {
  margin: 0.5rem 0 0.5rem 1rem;
}

.sidebar-nav a {
  display: block;
  color: var(--color-on-surface);
  text-decoration: none;
  margin-bottom: 0.25rem;
}

.sidebar-nav a:hover {
  color: var(--color-primary);
}

/* Sidebar Footer */
.sidebar-footer {
  padding: 1rem;
  text-align: center;
  background: var(--color-surface);
  border-top: 1px solid #ddd;
  font-size: 0.875rem;
}

/* Main Content */
.main {
  flex: 1;
  padding: 1rem;
  overflow-y: auto;
}
</style>

