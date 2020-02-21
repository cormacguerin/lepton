<template>
  <div class="input">
    <flex-col>
      <div class="margin">
        <input
          v-model="tableName"
          class="left"
          placeholder="dataset name"
        >
      </div>
      <textarea
        v-model="query"
        placeholder=""
      />
      <div class="error">
        {{ error }}
      </div>
      <flex-row>
        <div class="margin">
          <CButton
            class="active left"
            variant="outline"
            color="danger"
            @click="run"
          >
            Run Query
          </CButton>
        </div>
        <div class="margin">
          <CButton
            class="active left"
            variant="outline"
            color="danger"
            @click="save"
          >
            Save
          </CButton>
        </div>
        <div class="right">
          <div class="margin">
            <CButton
              class="active left"
              variant="outline"
              color="danger"
              @click="delete_"
            >
              Delete
            </CButton>
          </div>
        </div>
      </flex-row>
    </flex-col>
    <div class="preview">
      <CDataTable
        :items="columns"
        :fields="fields"
        small
        table-filter
        columns-per-page-select
        sorter
        pagination
      >
        <template
          #edit="{item, index}"
          :columns="columns"
        >
          <td class="py-2">
            <CButton
              color="primary"
              class="blue"
              variant="outline"
              square
              size="sm"
              @click="editTableColumn(item)"
            >
              Edit
            </CButton>
          </td>
        </template>
        <nav aria-label="pagination">
          <ul class="pagination" />
        </nav>
        <template #under-table="addTable" />
      </CDataTable>
    </div>
  </div>
</template>
<script>

export default {
  name: 'EditDataSetTable',
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
    dataset: {
      type: String,
      default: ''
    },
    query: {
      type: String,
      default: ''
    }
  },
  data () {
    return {
      dataTypes: [
        'serial', 'bigserial', 'int', 'bigint', 'decimal', 'bigdecimal', 'real', 'date', 'varchar_64', 'varchar_2048'
      ],
      columns: [
      ],
      fields: [
      ],
      error: ''
    }
  },
  watch: {
    dataset: function (val) {
      var ds = JSON.parse(val)
      console.log(ds)
      this.query = ds.query
      this.fields = ds.fields
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
    run () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/runQuery', {
        params: {
          database: vm.database,
          query: vm.query
        }
      })
        .then(function (response) {
          if (response.data) {
            console.log(response.data)
            if (response.data.status === 'success') {
              vm.error = ''
              console.log(response.data.message)
              vm.columns = response.data.message
              vm.fields = Object.keys(vm.columns[0])
            } else {
              vm.error = 'Error ' + response.data.message
              console.log(response.data.error)
            }
          }
        })
        .catch(function (error) {
          console.log(error)
        })
    },
    save () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/createDataSetTable', {
        params: {
          database: vm.database,
          table: vm.tableName,
          query: vm.query
        }
      })
        .then(function (response) {
          if (response.data) {
            console.log(response.data)
            if (response.data.status === 'success') {
              vm.error = ''
              vm.$parent.$parent.$parent.getDatabases(vm.tableName)
              vm.$parent.$parent.getTableSchema(vm.tableName)
            } else {
              vm.error = 'Error ' + response.data.message
              console.log(response.data.message)
            }
          }
        })
        .catch(function (error) {
          console.log(error)
        })
    },
    delete_ () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/deleteDataSetTable', {
        params: {
          database: vm.database,
          table: vm.tableName
        }
      })
        .then(function (response) {
          if (response.data) {
            console.log(response.data)
            if (response.data.status === 'success') {
              vm.$parent.$parent.$parent.getDatabases(vm.tableName)
              vm.$parent.$parent.collapse = false
            } else {
              vm.error = 'Error ' + response.data.message
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
.right {
  float: right;
}
.error {
  margin: 10px;
  padding-left: 10px;
  padding-right: 10px;
  color: #f86c6b;
  text-align: left;
}
input {
  text-indent: 10px;
  min-width: 100px;
  border: none;
  border-bottom: 2px solid #cfcfcf;
  outline: 0;
}
textarea {
  background-color: #efefef;
  margin: 10px;
  padding: 10px;
}
</style>
