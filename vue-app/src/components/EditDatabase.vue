<template>
  <div class="input">
    <input
      v-model="value"
      placeholder="new database name"
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
  name: 'EditDatabase',
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
    showModal () {
      this.$refs.myModalRef.show()
    },
    hideModal () {
      this.$refs.myModalRef.hide()
    },
    save () {
      var vm = this
      this.$axios.get('SERVER_URI + '/api/addDatabase', {
        params: {
          database: vm.value
        }
      })
        .then(function (response) {
          if (response.data) {
            console.log(response.data)
            if (response.data.status === 'success') {
              vm.$parent.$parent.getDatabases()
              vm.$parent.$parent.addDataModal = false
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
