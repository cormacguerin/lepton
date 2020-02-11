<template>
  <flex-col
    justify="left"
    class="header"
  >
    <Burger />

    <Sidebar>
      <ul class="sidebar-panel-nav">
        <li>
          <router-link to="/">
            Home
          </router-link>
        </li>
        <li>
          <router-link to="/data">
            Data
          </router-link>
        </li>
        <li>
          <router-link to="/visualize">
            Visualize
          </router-link>
        </li>
        <li>
          <router-link to="/crud">
            Crud
          </router-link>
        </li>
      </ul>
    </Sidebar>
  </flex-col>
</template>

<script>
import Burger from './Burger.vue'
import Sidebar from './Sidebar.vue'

export default {
  name: 'Header',
  components: {
    Burger,
    Sidebar
  },
  data () {
  },
  created () {
  },
  methods: {
  }
}
</script>

<style>
#header {
    font-family: "Avenir", Helvetica, Arial, sans-serif;
    -webkit-font-smoothing: antialiased;
    -moz-osx-font-smoothing: grayscale;
    text-align: left;
    height: 80px;
    color: #2c3e50;
    margin-top: 0px;
}
.header {
  background-color: #171f24;
}
ul.sidebar-panel-nav {
  list-style-type: none;
  text-align: left;
}
ul.sidebar-panel-nav > li > a {
  color: #fff;
  text-decoration: none;
  text-align: left;
  font-size: 1.5rem;
  display: block;
  padding-bottom: 0.5em;
}
</style>
