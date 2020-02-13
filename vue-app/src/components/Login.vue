<template>
  <flex-col
    justify="center"
    class="loginBox"
  >
    <input
      v-model="input.email"
      class="inputBox"
      type="text"
      name="email"
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
      class="buttonBox"
      type="button"
      @click="login()"
    >
      Login
    </button>
    <button
      class="buttonBox"
      type="button"
      @click="register()"
    >
      Register
    </button>
  </flex-col>
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
        email: '',
        password: ''
      }
    }
  },
  methods: {
    login () {
      var vm = this
      if (vm.input.email !== '' && vm.input.password !== '') {
        vm.$axios.post('https://' + this_SERVER_IP + '/login/', {
          email: vm.input.email,
          password: vm.input.password
        })
          .then(function (response) {
            if (response.data) {
              if (response.data.authorized === true) {
                vm.$emit('authenticated', true)
                location.reload()
              } else {
                console.log('Incorrect email and / password.')
              }
            }
          })
          .catch(function (error) {
            console.log(error)
          })
      } else {
        console.log('Please enter a email and password.')
      }
    },
    register () {
      var vm = this
      console.log(this.input)
      if (this.input.email !== '' && this.input.password !== '') {
        this.$axios.post('https://' + this_SERVER_IP + '/register/', {
          email: this.input.email,
          password: this.input.password
        })
          .then(function (response) {
            if (vm.input.email && vm.input.password) {
              this.$emit('authenticated', true)
            //  this.$router.replace({ name: 'secure' })
            } else {
              console.log('Incorrect email and / or password.')
            }
          })
          .catch(function (error) {
            console.log(error)
          })
      } else {
        console.log('Please enter a email and password.')
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
input.inputBox {
    margin: 5px;
    height: 30px;
    padding: 5px;
    outline: 0;
    border: 1px solid #efefef;
    border-radius: 3px;
}
button.buttonBox {
    margin: 5px;
    height: 40px;
    padding: 5px;
    border-style: none;
    border-radius: 3px;
    background-color: #022548;
    color: white;
}
.loginBox {
    width: 300px;
    margin-left: auto;
    margin-right: auto;
}
</style>
