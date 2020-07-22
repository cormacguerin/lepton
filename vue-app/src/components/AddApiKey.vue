<template>
  <div class="input">
    <flex-col>
      <div class="key">
        {{ apiKey }}
      </div>
      <div class="about">
        This is your API Key, store it somewhere secure for use with the API.
        You can not view this key again after this dialog has closed, but you
        can regenerate a new key anytime.
      </div>
      <flex-row>
        <input
          v-model="keyName"
          class="left"
          placeholder="api key name"
        >
        <div class="margin">
          <CButton
            class="active"
            color="info"
            @click="save"
          >
            Save
          </CButton>
        </div>
      </flex-row>
    </flex-col>
  </div>
</template>
<script>

export default {
  name: 'AddApiKey',
  components: {
  },
  props: {
    isOpen: {
      type: Boolean,
      default: false
    }
  },
  data () {
    return {
      keyName: '',
      apiKey: ''
    }
  },
  watch: {
    isOpen: function (val) {
      if (val === true) {
        this.apiKey = ''
        this.generateApiKey()
      }
    }
  },
  created () {
  },
  methods: {
    generateApiKey () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/generateApiKey', {
      })
        .then(function (response) {
          if (response.data.length === 35) {
            vm.apiKey = response.data
          }
        })
    },
    save () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/addApiKey', {
        params: {
          key_name: vm.keyName,
          key: vm.apiKey
        }
      })
        .then(function (response) {
          if (response.data) {
            console.log(response.data)
            if (response.data.status === 'success') {
              vm.$parent.$parent.getApiKeys()
              vm.$parent.$parent.addApiKeyModal = false
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
.margin {
  margin: 10px;
}
.key {
  margin: 10px;
  padding: 10px;
  background-color: #f5f5f5;
  border: 1px solid #efefef;
  color: #5f5f5f;
}
.nopadding {
  padding: 0px;
}
.left {
  float: left;
}
.about {
  margin: 30px;
  color: #5f5f5f;
}
input {
  margin: 10px;
  text-indent: 10px;
  width: 300px;
  border: none;
  border-bottom: 2px solid #cfcfcf;
  outline: 0;
}
</style>
