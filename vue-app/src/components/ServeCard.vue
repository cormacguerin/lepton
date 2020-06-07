<template>
  <div class="card">
    <flex-col
      no-wrap
      class="container"
    >
      <div class="database">
        <div class="flexgrow servetitle">
          {{ database }} - {{ table }}
        </div>
        <flex-col
          class="contentcontainer"
        >
          <flex-row class="info">
            <label
              class="switch"
            >
              <input
                type="checkbox"
                :checked="serving"
                @click="setServing(item)"
              >
              <span class="toggle" />
            </label>
            <div class="enabled">
              <div v-if="serving">
                Enabled
              </div>
              <div v-else>
                Disabled
              </div>
            </div>
            <div class="backendstatus">
              Backend Status : <span :class="statusclass">{{ getStatus() }}</span>
            </div>
          </flex-row>
        </flex-col>
      </div>
    </flex-col>
  </div>
</template>
<script>

export default {
  name: 'ServeCard',
  components: {
  },
  props: {
    database: {
      type: String,
      default: ''
    },
    table: {
      type: String,
      default: ''
    },
    column: {
      type: String,
      default: ''
    },
    displayfield: {
      type: String,
      default: ''
    },
    indexing: {
      type: Boolean,
      default: false
    },
    serving: {
      type: Boolean,
      default: false
    },
    total: {
      type: Number,
      default: 0
    },
    indexed: {
      type: Number,
      default: 0
    },
    refreshed: {
      type: Number,
      default: 0
    },
    servingstatus: {
      type: String,
      default: ''
    },
    loaded: {
      type: Object,
      default: function () {
        return {}
      }
    }
  },
  data () {
    return {
      statusclass: ''
    }
  },
  watch: {
    total: function (val) {
      this.$watch(
      )
    }
  },
  mounted () {
  },
  methods: {
    setServing () {
      var s
      if (this.serving === false || this.serving === null) {
        s = true
      } else {
        s = false
      }
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/setServing', {
        params: {
          database: vm.database,
          table: vm.table,
          serving: s
        }
      })
        .then(function (response) {
          if (response.data) {
            if (response.data.length === 0) {
              vm.serving = s
            }
          }
        })
    },
    getStatus () {
      if (this.servingstatus === 'serving') {
        this.statusclass = 'serving'
        return 'Serving'
      } else if (this.servingstatus === 'loading') {
        this.statusclass = 'loading'
        return 'Loading'
      } else if (this.servingstatus === 'shutdown') {
        this.statusclass = 'shutdown'
        return 'Shutdown'
      } else {
        this.statusclass = 'error'
        return 'No backend response'
      }
    }
  }
}
</script>

<style scoped>
.container {
  padding: 0px;
}
.absolute {
  position: absolute;
}
.card {
  margin-left: 10px;
  margin-right: 10px;
  margin-bottom: 10px;
  margin-top: 20px;
  border-radius: 3px;
  background-color: #fff;
  border: none;
}
.contentcontainer {
  border: 1px solid #efefef;
  color: #666;
}
.servetitle {
  width: 100%;
  border-radius-left-top: 3px;
  border-radius-right-top: 3px;
  text-align: left;
  background-color: #ffc107;
  padding: 5px 10px 5px 10px;
  color: #23282c;
  font-weight: bold;
}
.serving {
  color: #33b13d;
}
.loading {
  color: #ffc000;
}
.error {
  color: #ff5733;
}
.enabled {
  margin-left: 5px;
}
.backendstatus {
  margin-left: 10px;
}
.info {
  margin: 10px;
}
label.checkbox {
  cursor: pointer;
  -webkit-user-select: none;
  -moz-user-select: none;
  -ms-user-select: none;
  user-select: none;
}
.switch {
  position: relative;
  display: inline-block;
  width: 32px;
  height: 16px;
  margin: 2px;
}
.switch input {
  opacity: 0;
  width: 0;
  height: 0;
  border: 2px solid #efefef;
}
.toggle {
  position: absolute;
  cursor: pointer;
  top: 0;
  bottom: 0;
  left: 0;
  right: 0;
  background-color: #efefef;
  -webkit-transition: .4s;
  transition: .4s;
  border-radius: 5px;
}
.toggle:before {
  position: absolute;
  content: "";
  height: 12px;
  width: 16px;
  left: 2px;
  bottom: 2px;
  background-color: #fff;
  -webkit-transition: .4s;
  transition: .4s;
  border-radius: 5px;
  border: 2px solid #fff;
}
input:checked + .toggle {
  background-color: #2eadd3;
}
input:focus + .toggle {
  box-shadow: 0 0 1px #2196F3;
}
input:checked + .toggle:before {
  -webkit-transform: translateX(10px);
  -ms-transform: translateX(10px);
  transform: translateX(10px);
}

</style>
