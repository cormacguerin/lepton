<template>
  <div class="dropdown-item">
    <slot></slot>
  </div>
</template>

<script>

export default {
  name: 'CDropdownItem',
  components: {
  },
  data () {
    return {}
  },
  props: {
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
