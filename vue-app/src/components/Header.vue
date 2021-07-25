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
            <router-link to="/database">
              Data
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
            <router-link to="/apikeys">
              Api Keys
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
          src="../assets/wu2-64x51.png"
        >
        <flex-col
          justify="end"
        >
          <flex-row>
            <div
              class="nav-tab"
            >
              <h2
                class="nav-h2"
              >
                Data |
              </h2>
              <div class="nav-menu-container">
                <div class="nav-top-border">
                  <div class="nav-top" />
                </div>
                <div class="nav-menu">
                  <div
                    class="nav-item"
                    @click="gotoTab('database')"
                  >
                    Databases
                  </div>
                </div>
              </div>
            </div>
            <div
              class="nav-tab"
            >
              <h2
                class="nav-h2"
              >
                Chart |
              </h2>
              <div class="nav-menu-container">
                <div class="nav-top-border">
                  <div class="nav-top" />
                </div>
                <div class="nav-menu">
                  <div
                    class="nav-item"
                    @click="gotoTab('charts')"
                  >
                    Charts
                  </div>
                  <div
                    class="nav-item"
                    @click="gotoTab('dashboard')"
                  >
                    Dashboards
                  </div>
                </div>
              </div>
            </div>
            <div
              class="nav-tab"
            >
              <h2
                class="nav-h2"
              >
                Machine Learning |
              </h2>
              <div class="nav-menu-container">
                <div class="nav-top-border">
                  <div class="nav-top" />
                </div>
                <div class="nav-menu">
                  <div
                    class="nav-item"
                    @click="gotoTab('models')"
                  >
                    Models
                  </div>
                  <div
                    class="nav-item"
                    @click="gotoTab('inference')"
                  >
                    Inference
                  </div>
                </div>
              </div>
            </div>
            <div
              class="nav-tab"
            >
              <h2
                class="nav-h2"
              >
                Search |
              </h2>
              <div class="nav-menu-container">
                <div class="nav-top-border">
                  <div class="nav-top" />
                </div>
                <div class="nav-menu">
                  <div
                    class="nav-item"
                    @click="gotoTab('indexing')"
                  >
                    Indexing
                  </div>
                  <div
                    class="nav-item"
                    @click="gotoTab('serving')"
                  >
                    Serving
                  </div>
                  <div
                    class="nav-item"
                    @click="gotoTab('insights')"
                  >
                    Insights
                  </div>
                  <div
                    class="nav-item"
                    @click="gotoTab('crawler')"
                  >
                    Crawler
                  </div>
                </div>
              </div>
            </div>
            <div
              class="nav-tab"
            >
              <h2
                class="nav-h2"
              >
                Security |
              </h2>
              <div class="nav-menu-container">
                <div class="nav-top-border">
                  <div class="nav-top" />
                </div>
                <div class="nav-menu">
                  <div
                    class="nav-item"
                    @click="gotoTab('apikeys')"
                  >
                    Api Keys
                  </div>
                  <div
                    class="nav-item"
                    @click="gotoTab('account')"
                  >
                    Account
                  </div>
                </div>
              </div>
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
      id="database"
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
      id="indexing"
      hidden
    />
    <div
      id="serving"
      hidden
    />
    <div
      id="crawler"
      hidden
    />
    <div
      id="apikeys"
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
      selectedTab: 'database'
    }
  },
  created () {
  },
  methods: {
    gotoTab (t) {
      /*
      console.log(t)
      console.log(this.$refs[this.selectedTab])
      console.log(document.getElementById(this.selectedTab))
      document.getElementById(this.selectedTab).hidden = true
      document.getElementById(t).hidden = false
      */
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
  background-color: #2c3e50;
}
.desktopLogo {
  margin: 10px;
}
.nav-tab {
/*  font-family: 'Comfortaa', cursive;*/
  position: relative;
  display: inline-block;
  margin-left: 10px;
  font-size: 1em;
  color: white;
}
.nav-menu {
  color: #a0a0a0;
  background-color: #fdfdfd;
  border: 1px solid #ddd;
  margin-top: -1px;
  padding-left: 10px;
  padding-right: 10px;
  box-shadow: 3px 3px 12px 0px rgba(0,0,0,0.2);
}
.nav-menu-container {
  visibility: hidden;
  opacity: 0;
  width: 0px;
  margin-left: -20px;
  transition: visibility 0s, opacity 0.2s linear, width 0.2s, margin-left 0.2s ease-in-out;
  position: absolute;
  margin-top: -5px;
  z-index: 1;
}
.nav-tab:hover .nav-menu-container {
  visibility: visible;
  width: 110px;
  margin-left: 0px;
  opacity: 1;
}
.nav-item {
  display: block;
  text-align: left;
  width: 100px;
  margin: 5px;
  cursor: pointer;
  color: #707070;
}
.nav-top {
  height: 11px;
  width: 15px;
  margin-top: 1px;
  background-color: #fafbfb;
  clip-path: polygon(50% 0, 0 100%, 100% 100%);
}
.nav-top-border {
  height: 11px;
  width: 15px;
  margin-left: 10px;
  background-color: #707070;
  clip-path: polygon(50% 0, 0 100%, 100% 100%);
}
ul.sidebar-panel-nav {
  list-style-type: none;
  text-align: left;
  cursor: default;
}
h2.nav-h2 {
  font-size: 1em;
  cursor: default;
}
ul.sidebar-panel-nav > li > a {
  color: #fff;
  text-decoration: none;
  text-align: left;
  font-size: 1.5rem;
  display: block;
  padding-bottom: 0.5em;
  cursor: default;
}
</style>
