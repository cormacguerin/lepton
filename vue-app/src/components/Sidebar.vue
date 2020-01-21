<template>
  <div
    class="sidebar"
    @click="closeSidebarPanel"
  >
    <div
      v-if="isPanelOpen"
      class="sidebar-backdrop"
    />
    <transition name="slide">
      <div
        v-if="isPanelOpen"
        class="sidebar-panel"
      >
        <slot />
      </div>
    </transition>
  </div>
</template>
<script>
import { store, mutations } from '@/store.js'

export default {
  computed: {
    isPanelOpen () {
      return store.isNavOpen
    }
  },
  methods: {
    closeSidebarPanel: mutations.toggleNav
  }
}
</script>
<style>
    .slide-enter-active,
    .slide-leave-active
    {
        transition: transform 0.2s ease;
    }

    .slide-enter,
    .slide-leave-to {
        transform: translateX(-100%);
        transition: all 150ms ease-in 0s
    }

    .sidebar-backdrop {
        width: 100vw;
        height: 100vh;
        position: fixed;
        background-color: rgb(44,62,80,0.4);
        top: 0;
        left: 0;
        cursor: pointer;
    }

    .sidebar-panel {
        overflow-y: auto;
        background-color: #2c3e50;
        position: fixed;
        left: 0;
        top: 0;
        height: 100vh;
        z-index: 999;
        padding: 3rem 20px 2rem 20px;
        width: 300px;
    }
</style>
