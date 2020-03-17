<template>
  <div class="input">
    <flex-col>
      <div class="margin">
        <input
          v-model="tableName"
          class="left"
          placeholder="table name"
        >
      </div>
      <flex-row>
        <div class="margin">
          <input
            v-model="columnName"
            placeholder="primary key"
          >
        </div>
        <div class="margin nopadding">
          <CDropdown
            ref="dataTypeDropDown"
            :toggler-text="dataType"
            title="data type"
          >
            <CDropdownItem
              v-for="dt in dataTypes"
              :key="dt"
              @click.native="selectDataType(dt)"
            >
              {{ dt }}
            </CDropdownItem>
          </CDropdown>
        </div>
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
  name: 'AddScope',
  components: {
  },
  props: {
    database: {
      type: String,
      default: ''
    },
    tableName: {
      type: String,
      default: ''
    },
    columnName: {
      type: String,
      default: ''
    }
  },
  data () {
    return {
      scopeTypes: [
        'database', 'chart', 'search'
      ],
      databases: [],
      selectedType: '',
      selectedSource: ''
    }
  },
  created () {
  },
  methods: {
    selectDataType (dt) {
      this.$refs.dataTypeDropDown.hide()
      this.dataType = dt
    },
    save () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/saveScope', {
        params: {
          database: vm.database,
          table: vm.tableName,
          column: vm.columnName,
          datatype: vm.dataType
        }
      })
        .then(function (response) {
          if (response.data) {
            console.log(response.data)
            if (response.data.status === 'success') {
              vm.$parent.$parent.$parent.getDatabases(vm.tableName)
              // vm.$parent.$parent.getTableSchema(vm.tableName)
              vm.$parent.$parent.addTableModal = false
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
.nopadding {
  padding: 0px;
}
.left {
  float: left;
}
input {
  margin: 10px;
  text-indent: 10px;
  min-width: 100px;
  border: none;
  border-bottom: 2px solid #cfcfcf;
  outline: 0;
}
</style>
