<template>
  <div id="app">
    <div v-if="userinfo.authorized === false">
      <div class="headerContainer">
        <div class="layout">
          <flex-row>
            <div>
              <img
                alt="logo"
                src="./assets/uchan_new_128.png"
              >
            </div>
            <flex-col class="logo">
              compudeep
            </flex-col>
          </flex-row>
        </div>
      </div>
      <Login msg="Login" />
    </div>
    <div v-else>
      <ControlPanel msg="ControlPanel" />
      <router-view />
    </div>
  </div>
</template>

<script>
import Login from './components/Login.vue'
import ControlPanel from './components/ControlPanel.vue'

export default {
  name: 'App',
  components: {
    Login,
    ControlPanel
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
  watch: {
    $route (to, from) {
      this.show = false
      console.log('to')
      console.log(to)
      console.log('from')
      console.log(from)
      console.log('this.$route')
      console.log(this.$route)
      console.log(' - - - ')
    }
  },
  created () {
    this.getUserInfo()
  },
  methods: {
    getUserInfo () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/getUserInfo', {
      })
        .then(function (response) {
          console.log(response)
          if (response.data) {
            console.log(response.data)
            // this.userinfo = Object.assign({}, this.userinfo, response.data)
            vm.userinfo = response.data
            console.log(vm.userinfo)
          }
        })
        .catch(function (error) {
          console.log(error)
        })
    }
  }
}
</script>

<style>
#app {
  font-family: 'Avenir', Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
  background-color: #fafbfb;
}
.logo {
  color: white;
  margin: 10px;
  font-size: 2em;
  font-family: 'Comfortaa', cursive;
  justify-content: flex-end;
}
.layout {
  width:350px;
  margin-left: auto;
  margin-right: auto;
}
.headerContainer {
  margin-top: 100px;
  margin-bottom: 20px;
  min-width: 100%;
  padding-top: 20px;
  padding-bottom: 20px;
  background-color: #2c3e50;
}
body {
  background-color: white;
}
</style>
