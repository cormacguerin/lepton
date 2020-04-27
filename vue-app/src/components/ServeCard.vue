<template>
  <div class="card">
    <flex-col
      no-wrap
      class="container"
    >
      <div class="database">
        <flex-row
          class="left"
        >
          <div class="flexgrow servetitle">
            {{ database }} - {{ table }}
          </div>
        </flex-row>
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
              Serving: Enabled
            </div>
            <div v-else>
              Serving: Disabled
            </div>
          </div>
          <div>
            {{ status }}
          </div>
          <div>
            {{ loaded }}
          </div>
        </flex-row>
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
    status: {
      type: Object,
      default: function () {
        return {}
      }
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
  min-height: 125px;
  margin-left: 10px;
  margin-right: 10px;
  margin-bottom: 10px;
  margin-top: 20px;
  border-radius: 3px;
  background-color: #fff;
  -webkit-box-shadow:0 2px 4px 0 #b2b5be;
     -moz-box-shadow:0 2px 4px 0 #b2b5be;
          box-shadow:0 2px 4px 0 #b2b5be;
}
.servetitle {
  width: 100%;
  border-radius-left-top: 3px;
  border-radius-right-top: 3px;
  text-align: left;
  background-color: #ffc107;
  padding: 5px;
  color: #23282c;
  font-weight: bold;
}
.enabled {
  margin-left: 5px;
  color: #333333;
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

/* Hide default HTML checkbox */
.switch input {
  opacity: 0;
  width: 0;
  height: 0;
  border: 2px solid #efefef;
}

/* The toggle */
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
