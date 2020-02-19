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
      <textarea v-model="query"
        placeholder=""
      >
      </textarea>
      <div class="margin">
        <CButton
          class="active left"
          color="info"
          @click="run"
        >
          Run Query
        </CButton>
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
    </flex-col>
    <div class="preview">
      <CDataTable
        :items="columns"
        :fields="fields"
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
  name: 'AddDataSetTable',
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
      ]
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
              console.log(response.data.message)
              vm.columns = response.data.message
              vm.fields = Object.keys(vm.columns[0])
            } else {
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
              vm.$parent.$parent.$parent.getDatabases(vm.tableName)
              vm.$parent.$parent.getTableSchema(vm.tableName)
              vm.$parent.$parent.addDataSetTableModal = false
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
