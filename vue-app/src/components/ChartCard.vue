<template>
  <div class="card">
    <!--
    <flex-col
      no-wrap
      class="container"
    >
      <div class="database">
        <flex-row
          class="left"
        >
          <div
            class="database-icon"
          >
            <div class="cylinder" />
            <div class="cylinder" />
            <div class="cylinder" />
          </div>
          <div class="flexgrow">
            <h2>{{ database }}</h2>
          </div>
          <div class="edit">
            <CDropdown
              nav
              placement="bottom-end"
            >
              <template #toggler="toggler">
                <i
                  class="fa fa-ellipsis-v pointer"
                  aria-hidden="true"
                />
              </template>
              <CDropdownItem
                @click="deleteDB()"
              >
                Delete
              </CDropdownItem>
              <CDropdownItem>Edit</CDropdownItem>
            </CDropdown>
          </div>
        </flex-row>
        <flex-col
          justify="end"
          class="centerflex"
        >
          <flex-row
            justify="start"
            class="buttons"
          >
            <CButton
              v-for="t in tables"
              :key="t.tablename"
              :color="getTableColor(t.search)"
              class="tablebutton"
              variant="outline"
              @click="getTableSchema(t.tablename)"
            >
              {{ t.tablename }}
            </CButton>
            <CButton
              color="info"
              class="tablebutton active"
              @click="addTableModal = true"
            >
              <span>
                <i
                  class="fa
                  fa-plus"
                  aria-hidden="true"
                />
                Data Table
              </span>
            </CButton>
            <CModal
              title="Add Table"
              color="info"
              :show.sync="addTableModal"
            >
              <template #footer-wrapper>
                <div class="hidden" />
              </template>
              <AddTable
                :key="index"
                :database="database"
              />
            </CModal>
            <CButton
              class="tablebutton active"
              color="warning"
              variant="outline"
              @click="addSearchTableModal = true"
            >
              <span>
                <i
                  class="fa fa-plus"
                  aria-hidden="true"
                />
              </span>
              Search Table
            </CButton>
            <CModal
              title="Add Search Table"
              color="warning"
              :show.sync="addSearchTableModal"
            >
              <template #footer-wrapper>
                <div class="hidden" />
              </template>
              <AddSearchTable
                :key="index"
                :database="database"
              />
            </CModal>
          </flex-row>
        </flex-col>
      </div>
    </flex-col>
    <CCollapse
      :show="collapse"
      :duration="300"
      class="mt-2"
    >
      <CModal
        title="Edit Table"
        color="info"
        :show.sync="editTableColumnModal"
      >
        <template #footer-wrapper>
          <div class="hidden" />
        </template>
        <EditTableColumn
          :database="database"
          :table-name="selectedTable"
          :column-name="itemData.column_name"
          :edit-column-name="itemData.column_name"
          :data-type="itemData.data_type"
          :edit-data-type="itemData.data_type"
          :columns="columns"
          is-edit
        />
      </CModal>
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
        <template #under-table="addTable">
          <div class="addTable">
            <CButton
              class="btn active margin-right"
              color="info"
              @click="deleteTable"
            >
              <span>
                <i
                  class="fa fa-minus"
                  aria-hidden="true"
                />
                Delete
              </span>
            </CButton>
            <CButton
              class="btn active"
              color="info"
              @click="addTableColumnModal = true"
            >
              <span>
                <i
                  class="fa fa-plus"
                  aria-hidden="true"
                />
                Column
              </span>
            </CButton>
            <CModal
              title="Add Table"
              color="info"
              :show.sync="addTableColumnModal"
            >
              <template #footer-wrapper>
                <div class="hidden" />
              </template>
              <EditTableColumn
                :key="index"
                :database="database"
                :table-name="selectedTable"
                :columns="columns"
              >
                ADD COLUMN
              </EditTableColumn>
            </CModal>
          </div>
        </template>
      </CDataTable>
    </CCollapse>
    -->
  </div>
</template>
<script>

// import EditTableColumn from './EditTableColumn.vue'

export default {
  name: 'ChartCard',
  components: {
    // EditTableColumn
  },
  props: {
    database: {
      type: String,
      default: function () {
        return { }
      }
    },
    tables: {
      type: Array,
      default: function () {
        return []
      }
    }
  },
  data () {
    return {
      columns: [
      ],
      fields: [
      ],
      details: [
      ],
      selectedTable: '',
      collapse: false,
      addTableModal: false,
      addSearchTableModal: false,
      addTableColumnModal: false,
      editTableColumnModal: false,
      itemData: {
        database: '',
        table_name: '',
        column_name: '',
        data_type: '',
        columns: []
      }
    }
  },
  created () {
  },
  methods: {
    getTableColor (t) {
      console.log(t)
      if (t === true) {
        return 'warning'
      } else {
        return 'info'
      }
    },
    editTableColumn (i) {
      this.editTableColumnModal = true
      this.itemData = i
    },
    getTableSchema (table) {
      if (table === this.selectedTable && this.collapse === true) {
        this.collapse = false
        return
      }
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/getTableSchema', {
        params: {
          database: vm.database,
          table: table
        }
      })
        .then(function (response) {
          if (response.data) {
            vm.selectedTable = table
            vm.columns = response.data.d
            console.log(1)
            console.log(vm.columns)
            console.log(vm.fields)
            if (vm.columns.length === 0) {
              console.log(2)
              vm.collapse = true
              return
            }
            console.log(3)
            vm.fields = Object.keys(vm.columns[0])
            vm.fields.push({
              key: 'edit',
              label: '',
              _style: 'width:1%',
              sorter: false,
              filter: false
            })
            vm.collapse = true
          }
        })
        .catch(function (error) {
          console.log(error)
        })
    },
    deleteTable () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/deleteTable', {
        params: {
          database: vm.database,
          table: vm.selectedTable
        }
      })
        .then(function (response) {
          if (response.data) {
            if (response.data.status === 'success') {
              vm.$parent.getCharts()
              vm.collapse = false
            }
          }
        })
    },
    deleteDB () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/deleteChart', {
        params: {
          database: vm.database
        }
      })
        .then(function (response) {
          if (response.data) {
            if (response.data.status === 'success') {
              vm.$parent.getCharts()
            }
          }
        })
    }
  }
}
</script>

<style scoped>
h2 {
    padding-top: 30px;
    color: #fff;
    text-align: left;
    font-size: 1.8em;
}
.database-icon {
    padding:20px;
    margin-right: 20px;
    transform: rotate(180deg);
    width: 120px;
}
.cylinder {
    margin-top: -5px;
    width: 100%;
    height: 20px;
    border-top: 2px solid #4e97ad;
    border-bottom: 1px solid #efefef;
    border-radius: 50%;
    background-color: #fff;
    -webkit-box-shadow:0 -4px 4px 0px rgba(0, 0, 0, 0.3), 0 0 0px rgba(0, 0, 0, 0.1) inset;
       -moz-box-shadow:0 -4px 4px 0px rgba(0, 0, 0, 0.3), 0 0 0px rgba(0, 0, 0, 0.1) inset;
            box-shadow:0 -4px 4px 0px rgba(0, 0, 0, 0.3), 0 0 0px rgba(0, 0, 0, 0.1) inset;
}
.buttons {
  margin-left: 10px;
  margin-right: 10px;
}
.btn {
  color: white;
}
.blue {
  color: #39b2d5;
}
.tablebutton {
  color: white;
  margin: 10px;
}
.container {
    padding: 0px;
}
.card {
    min-height: 125px;
    margin: 10px;
    border: 1px solid #fff;
    border-radius: 3px;
    background-color: #fff;
    /*
      box-shadow: 0px 1px 5px rgba(0,0,0,0.3), 0 0px 0px rgba(0,0,0,0.22);
    */
}
.database {
    min-width: 100px;
    width: 100%;
    background-color: #07202f;
    border-top: #39b2d5 4px solid;
    border-radius: 3px;
    color: white;
}
.left {
    min-width: 100px;
}
.margin-right {
  margin-right: 10px;
}
.edit {
  margin-right: 5px;
  margin-left: 5px;
  width: 10px;
}
.flexgrow {
    flex-grow: 2;
}
.info {
    width: 75%;
}
.centerflex {
  width: 100%;
}
.addTable {
    float: right;
}
.pointer {
    cursor: pointer;
}
</style>
