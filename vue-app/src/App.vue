<template>
  <div id="app">
    <img
      alt="Vue logo"
      src="./assets/logo.png"
    >
    <div v-if="userinfo.authorized === false">
      <Login msg="Login" />
    </div>
    <div v-else>
      <div>logged in</div>
    </div>
  </div>
</template>

<script>
import Login from './components/Login.vue'

export default {
  name: 'App',
  components: {
    Login
  },
  data () {
    return {
      userinfo: {
        authorized: false,
        language: '',
        region: ''
      }
    }
  },
  beforeMount () {
    this.getUserInfo()
  },
  methods: {
    getUserInfo () {
      var vm = this
      this.$axios.get('https://35.239.29.200/api/getUserInfo', {
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
    margin-top: 60px;
}
</style>
