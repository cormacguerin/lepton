<template>
  <div class="dropdown-item">
    <slot />
  </div>
</template>

<script>

export default {
  name: 'CDropdownItem',
  components: {
  },
  props: {
  },
  data () {
    return {}
  },
  created () {
  },
  methods: {
  }
}
</script>

<style>
@media (max-width: 576px) {
  .desktopNav {
    display: none!important;
  }
}
@media (min-width: 576px) {
  .mobileNav {
    display: none!important;
  }
}
.dropdown-item {
  display: block;
  border: none;
}
</style>
