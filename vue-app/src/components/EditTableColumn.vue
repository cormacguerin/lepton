<template>
  <div class="input">
    <flex-row>
      <div class="margin">
        <input
          v-model="columnName"
          placeholder="column name"
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
    </flex-row>
    <div class="margin left">
      <CButton
        class="active"
        color="info"
        @click="save"
      >
        Save
      </CButton>
    </div>
    <template v-if="isEdit === true">
      <div class="margin right">
        <CButton
          class="active"
          color="info"
          @click="deleteColumn"
        >
          Delete
        </CButton>
      </div>
    </template>
  </div>
</template>
<script>

export default {
  name: 'EditTable',
  components: {
  },
  props: {
    isEdit: {
      type: Boolean,
      default: false
    },
    database: {
      type: String,
      default: ''
    },
    tableName: {
      type: String,
      default: ''
    },
    tableType: {
      type: String,
      default: ''
    },
    columnName: {
      type: String,
      default: ''
    },
    editColumnName: {
      type: String,
      default: ''
    },
    dataType: {
      type: String,
      default: ''
    },
    editDataType: {
      type: String,
      default: ''
    },
    displayField: {
      type: String,
      default: ''
    },
    columns: {
      type: Array,
      default: function () {
        return []
      }
    }
  },
  data () {
    return {
      dataTypes: [
        'serial', 'bigserial', 'int', 'bigint', 'decimal', 'bigdecimal', 'real', 'date', 'varchar_64', 'varchar_2048', 'text'
      ],
      fts: false
    }
  },
  created () {
    if (!this.dataType) {
      this.dataType = 'serial'
    }
  },
  methods: {
    selectDataType (dt) {
      this.$refs.dataTypeDropDown.hide()
      this.dataType = dt
      if (dt === 'text') {
        this.displayField = this.columns[0].column_name
      }
    },
    selectDisplayField (t) {
      this.$refs.displayFieldDropDown.hide()
      this.displayField = t
    },
    save () {
      var vm = this
      var params = {
        database: vm.database,
        table: vm.tableName,
        column: vm.columnName,
        datatype: vm.dataType
      }
      var saveUrl
      if (this.isEdit === false) {
        saveUrl = this.$SERVER_URI + '/api/addTableColumn'
      } else {
        saveUrl = this.$SERVER_URI + '/api/updateTableColumn'
        params.editColumn = vm.editColumnName
        params.editDatatype = vm.editDataType
      }
      this.$axios.get(saveUrl, {
        params: params
      })
        .then(function (response) {
          if (response.data) {
            console.log(response.data)
            if (response.data.status === 'success') {
              if (vm.isEdit === false) {
                vm.$parent.$parent.$parent.$parent.getTableSchema(vm.tableName, true)
                vm.$parent.$parent.$parent.$parent.addTableColumnModal = false
              } else {
                vm.$parent.$parent.$parent.getTableSchema(vm.tableName, true)
                vm.$parent.$parent.$parent.editTableColumnModal = false
              }
            } else {
              console.log(response.data.message)
            }
          }
        })
        .catch(function (error) {
          console.log(error)
        })
    },
    deleteColumn () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/deleteColumn', {
        params: {
          database: vm.database,
          table: vm.tableName,
          column: vm.columnName
        }
      })
        .then(function (response) {
          if (response.data) {
            console.log(response.data)
            if (response.data.status === 'success') {
              vm.$parent.$parent.$parent.getTableSchema(vm.tableName, true)
              vm.$parent.$parent.$parent.editTableColumnModal = false
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
.right {
  float: right;
}
input {
  margin-top: 10px;
  margin-bottom: 10px;
  min-width: 100px;
  border: none;
  border-bottom: 2px solid #cfcfcf;
  outline: 0;
}
.displayfield {
  margin-top: 15px;
  margin-bottom: 15px;
  margin-left: 10px;
}
.container {
  display: block;
  position: relative;
  padding-left: 35px;
  margin-bottom: 12px;
  cursor: pointer;
  -webkit-user-select: none;
  -moz-user-select: none;
  -ms-user-select: none;
  user-select: none;
}

.container input {
  position: absolute;
  opacity: 0;
  cursor: pointer;
  height: 0;
  width: 0;
}

.checkmark {
  position: absolute;
  top: 0;
  left: 0;
  height: 20px;
  width: 20px;
  background-color: #eee;
}

.container input:checked ~ .checkmark {
  background-color: #39b2d5;
}

.checkmark:after {
  content: "";
  position: absolute;
  display: none;
}

.container input:checked ~ .checkmark:after {
  display: block;
}

.container .checkmark:after {
  left: 9px;
  top: 5px;
  width: 5px;
  height: 10px;
  border: solid white;
  border-width: 0 3px 3px 0;
  -webkit-transform: rotate(45deg);
  -ms-transform: rotate(45deg);
  transform: rotate(45deg);
}
</style>
