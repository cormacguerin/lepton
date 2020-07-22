<template>
  <div id="something">
    <Header msg="header" />
    <div class="view">
    </div>
  </div>
</template>

<script>
import Header from './Header.vue'

export default {
  name: 'ControlPanel',
  components: {
    Header
  },
  data () {
    return {
      userinfo: {
        authorized: '',
        language: '',
        region: ''
      }
    }
  },
  created () {
  },
  methods: {
  }
}
</script>

<style scoped>
.control-panel {
    font-family: 'Avenir', Helvetica, Arial, sans-serif;
    -webkit-font-smoothing: antialiased;
    -moz-osx-font-smoothing: grayscale;
    text-align: left;
    color: #2c3e50;
}
.view {
    display: block;
    height: 100%;
    width: 100%;
}
</style>
