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
            <router-link to="/">
              Home
            </router-link>
          </li>
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
        </ul>
      </Sidebar>
    </div>
    <div
      class="desktopNav"
    >
      <CNav
        variant="tabs"
        class="tabs"
        fade
      >
        <div
          @click="showTab('home')"
        >
          <CNavItem
            class="homeTab"
            title="Home"
          >
            <h2>Home</h2>
          </CNavItem>
        </div>
        <div
          @click="showTab('schema')"
        >
          <CNavItem
            class="schemaTab"
            title="Schema"
          >
            <h2>Schema</h2>
          </CNavItem>
        </div>
        <div
          @click="showTab('dashboard')"
        >
          <CNavItem
            class="dashboardTab"
            title="Dashboard"
          >
            <h2>Dashboard</h2>
          </CNavItem>
        </div>
        <div
          @click="showTab('insights')"
        >
          <CNavItem
            class="insightsTab"
            title="Insights"
          >
            <h2>Insights</h2>
          </CNavItem>
        </div>
      </CNav>
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
