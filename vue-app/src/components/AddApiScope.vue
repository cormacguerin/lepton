<template>
  <div class="input">
    <flex-col justify="start">
      <flex-row>
        <div class="margin nopadding">
          <CDropdown
            ref="apiDropDown"
            :toggler-text="selectedApi"
            title="api"
          >
            <CDropdownItem
              v-for="a in apis"
              :key="a"
              @click.native="selectApi(a)"
            >
              {{ a }}
            </CDropdownItem>
          </CDropdown>
        </div>
        <div class="margin nopadding">
          <CDropdown
            ref="databaseDropDown"
            :toggler-text="selectedDatabase"
            title="database"
          >
            <CDropdownItem
              v-for="d in dbs"
              :key="d.key"
              @click.native="selectDatabase(d)"
            >
              {{ d.key }}
            </CDropdownItem>
          </CDropdown>
        </div>
        <div class="margin nopadding">
          <CDropdown
            ref="tableDropDown"
            :toggler-text="selectedTable"
            title="table (optional)"
          >
            <CDropdownItem
              v-for="t in tables"
              :key="t"
              @click.native="selectTable(t)"
            >
              {{ t }}
            </CDropdownItem>
          </CDropdown>
        </div>
      </flex-row>
      <flex-row
        justify="start"
        class="margin"
      >
        <CButton
          class="active"
          color="info"
          @click="save"
        >
          Save
        </CButton>
      </flex-row>
    </flex-col>
  </div>
</template>
<script>

export default {
  name: 'AddApiScope',
  components: {
  },
  props: {
    isOpen: {
      type: Boolean,
      default: false
    },
    dbs: {
      type: Object,
      default: function () {
        return {}
      }
    },
    apiKey: {
      type: Number,
      default: null
    }
  },
  data () {
    return {
      apis: ['data', 'chart', 'search', 'model'],
      selectedApi: 'data',
      SelectedDatabase: 'select',
      SelectedTable: 'select',
      tables: []
    }
  },
  watch: {
    isOpen: function (val) {
      if (val === true) {
        this.selectedApi = 'select'
        this.selectedDatabase = 'select'
        this.selectedTable = 'select'
      }
    }
  },
  created () {
  },
  methods: {
    validateSave () {
      if (this.selectedApi === 'select') {
        return false
      }
      if (this.selectedDatabase === 'select' && !(this.selectedApi === 'model')) {
        return false
      }
      if (this.selectedTable === 'select') {
        this.selectedTable = ''
      }
      return true
    },
    save () {
      if (this.validateSave() === false) {
        return
      }
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/addApiScope', {
        params: {
          key_id: vm.apiKey,
          api: vm.selectedApi,
          database: vm.selectedDatabase,
          table: vm.selectedTable
        }
      })
        .then(function (response) {
          if (response.data) {
            console.log(response.data)
            if (response.data.status === 'success') {
              vm.$parent.$parent.getApiKeys()
              vm.$parent.$parent.addApiScopeModal = false
            } else {
              console.log(response.data.message)
            }
          }
        })
        .catch(function (error) {
          console.log(error)
        })
    },
    selectApi (a) {
      this.selectedApi = a
    },
    selectDatabase (d) {
      this.tables = []
      if (d.key) {
        this.selectedDatabase = d.key
        for (var t in d.tables) {
          this.tables.push(d.tables[t].tablename)
        }
      }
    },
    selectTable (t) {
      if (t) {
        this.selectedTable = t
      }
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
