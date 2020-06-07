<template>
  <div id="app">
    <div v-if="userinfo.authorized === false">
      <div class="headerContainer">
        <div class="layout">
          <flex-col class="container">
            <div class="logo">
              <img
                alt="logo"
                src="./assets/wu2-100x125.png"
              >
            </div>
            <div class="title">
              compdeep
            </div>
            <Login msg="Login" />
          </flex-col>
        </div>
      </div>
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

<style scoped>
body {
  background-color: #fafbfb;
}
#app {
  font-family: 'Avenir', Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
  background-color: #fafbfb;
}
.container {
  border: 1px solid #efefef;
  border-radius: 5px;
  background-color: #fff;
  padding: 20px;
  flex-justify: center;
}
.logo {
  background-color: #9570d3;
  border-radius: 50%;
  width: 200px;
  height: 200px;
  margin-left: auto;
  margin-right: auto;
  padding-top: 50px;
}
.title {
  color: white;
  margin: 10px;
  font-size: 2em;
  font-family: 'Comfortaa', cursive;
  justify-content: flex-end;
  color: #2c3e50;
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
}
body {
  background-color: #fafafa;
}
</style>
