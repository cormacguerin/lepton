<template>
  <flex-row class="loginBox">
    <input
      v-model="input.username"
      class="inputBox"
      type="text"
      name="username"
      placeholder="Username"
    >
    <input
      v-model="input.password"
      class="inputBox"
      type="password"
      name="password"
      placeholder="Password"
    >
    <button
      type="button"
      @click="login()"
    >
      Login
    </button>
  </flex-row>
</template>

<script>
export default {
  name: 'Login',
  props: {
    msg: {
      type: String,
      default: function () {
        return { message: '' }
      }
    }
  },
  data () {
    return {
      input: {
        username: '',
        password: ''
      }
    }
  },
  methods: {
    login () {
      if (this.input.username != '' && this.input.password != '') {
        if (this.input.username == this.$parent.account.username && this.input.password == this.$parent.account.password) {
          this.$emit('authenticated', true)
          this.$router.replace({ name: 'secure' })
        } else {
          console.log('Incorrect username and / or password.')
        }
      } else {
        console.log('Please enter a username and password.')
      }
    }
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
h3 {
    margin: 40px 0 0;
}
ul {
    list-style-type: none;
    padding: 0;
}
li {
    display: inline-block;
    margin: 0 10px;
}
a {
    color: #42b983;
}
</style>
