<template>
  <div class="input">
    <flex-row>
      <div class="margin">
        <input
          v-model="tableName"
          class="left"
          placeholder="table name"
        >
      </div>
      <div class="margin">
        <CButton
          class="active left"
          color="info"
          @click="save"
        >
          Save
        </CButton>
      </div>
    </flex-row>
  </div>
</template>
<script>

export default {
  name: 'AddSearchTable',
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
        'serial', 'bigserial', 'int', 'bigint', 'decimal', 'bigdecimal', 'real', 'date', 'varchar_64', 'varchar_2048'
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
      this.$axios.get(this.$SERVER_URI + '/api/createSearchTable', {
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
              vm.$parent.$parent.addSearchTableModal = false
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
.thinmargin {
  margin-left: 10px;
  margin-right: 10px;
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
