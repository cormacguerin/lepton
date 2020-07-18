<template>
  <div class="input">
    <input
      v-model="value"
      placeholder="model name"
    >
    <CButton
      class="btn active"
      color="info"
      @click="save"
    >
      Save
    </CButton>
  </div>
</template>
<script>

export default {
  name: 'EditModel',
  components: {
  },
  props: {
    value: {
      type: String,
      default: ''
    }
  },
  created () {
  },
  methods: {
    save () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/addModel', {
        params: {
          model: vm.value,
          language: 'python'
        }
      })
        .then(function (response) {
          if (response.data) {
            console.log(response.data)
            if (response.data.status === 'success') {
              vm.$parent.$parent.getModels()
              vm.$parent.$parent.addModelModal = false
            } else {
              console.log(response.data.message)
            }
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
h2 {
    width: 100%;
    padding: 15px 0px 0px 0px;
    margin: 0px;
    color: #777;
    text-align: center;
    font-size: 1.2em;
}
.database-icon {
    transform: rotate(180deg);
    width: 100%;
}
.btn {
  margin: 10px;
}
input {
  margin: 10px;
  width: 300px;
  border: none;
  border-bottom: 2px solid #cfcfcf;
  outline: 0;
}
</style>
