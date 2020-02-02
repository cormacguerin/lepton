<template>
  <div class="input">
    <flex-col>
      <div class="margin">
        <input
          class="left"
          v-model="tableName"
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
            color="text-white bg-dark"
            no-caret
            nav
            placement="bottom-end"
          >
            <CDropdownItem
              v-for="dt in dataTypes"
              :key="dt"
              @click="selectDataType(dt)"
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
  name: 'AddTable',
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
      dataTypes: [
        'serial', 'bigserial', 'int', 'bigint', 'decimal', 'bigdecimal', 'real', 'date', 'varchar_64', 'varchar_2048', 'text'
      ],
      dataType: 'serial'
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
    selectDataType (dt) {
      this.$refs.dataTypeDropDown.hide()
      this.dataType = dt
    },
    save () {
      var vm = this
      this.$axios.get('https://35.239.29.200/api/createTable', {
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
              vm.$parent.$parent.getTableSchema(vm.tableName)
              vm.$parent.$parent.createTableModal = false
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
  min-width: 100px;
  border: none;
  border-bottom: 2px solid #cfcfcf;
  outline: 0;
}
</style>
