<template>
  <flex-col
    justify="left"
    class="header"
  >
    <div class="mobileNav">
      <Burger />

      <Sidebar>
        <ul class="sidebar-panel-nav">
          <li>
            <router-link to="/schema">
              Schema
            </router-link>
          </li>
          <li>
            <router-link to="/dashboard">
              Dashboard
            </router-link>
          </li>
          <li>
            <router-link to="/insights">
              Insights
            </router-link>
          </li>
          <li>
            <router-link to="/configure">
              Configure
            </router-link>
          </li>
        </ul>
      </Sidebar>
    </div>
    <div
      class="desktopNav"
    >
      <flex-row>
        <img
          class="desktopLogo"
          alt="logo"
          src="../assets/uchan_new_64.png"
        >
        <flex-col
          justify="end"
        >
          <flex-row>
            <div
              class="tab"
              @click="showTab('schema')"
            >
              <h2>Schema |</h2>
            </div>
            <div
              class="tab"
              @click="showTab('dashboard')"
            >
              <h2>Dashboard |</h2>
            </div>
            <div
              class="tab"
              @click="showTab('insights')"
            >
              <h2>Insights |</h2>
            </div>
            <div
              class="tab"
              @click="showTab('configure')"
            >
              <h2>Configure |</h2>
            </div>
          </flex-row>
        </flex-col>
      </flex-row>
    </div>
    <div
      id="home"
      hidden
    />
    <div
      id="schema"
      hidden
    />
    <div
      id="dashboard"
      hidden
    />
    <div
      id="insights"
      hidden
    />
    <div
      id="configure"
      hidden
    />
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
    return {
      selectedTab: 'schema'
    }
  },
  created () {
  },
  methods: {
    showTab (t) {
      console.log(t)
      console.log(this.$refs[this.selectedTab])
      console.log(document.getElementById(this.selectedTab))
      document.getElementById(this.selectedTab).hidden = true
      document.getElementById(t).hidden = false
      this.selectedTab = t
      this.$router.push({ path: '/' + t, name: t })
    }
  }
}
</script>

<style>
@media (max-width: 576px) {
  .desktopNav {
    display: none!important;
    background-color: #2c3e50;
    height: 100px;
  }
}
@media (min-width: 576px) {
  .mobileNav {
    display: none!important;
  }
}
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
.desktopLogo {
  margin: 10px;
}
.tab {
  font-family: 'Comfortaa', cursive;
  margin-left: 10px;
  font-size: 1em;
  color: white;
  cursor: pointer;
}
.nav-tabs {
  margin-bottom: 0px;
}
.tabsContainer {
  justify-content: flex-end;
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
h2 {
  font-size: 1em;
}
</style>
