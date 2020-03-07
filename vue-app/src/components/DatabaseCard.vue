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
          <div
            class="database-icon"
          >
            <div class="cylinder" />
            <div class="cylinder" />
            <div class="cylinder" />
          </div>
          <div class="flexgrow">
            <h2>{{ formatDatabaseName() }}</h2>
          </div>
          <div class="edit">
            <CDropdown>
              <template #toggler="toggler">
                <i
                  class="fa fa-ellipsis-v pointer"
                  aria-hidden="true"
                />
              </template>
              <CDropdownItem
                @click.native="deleteDB()"
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
              :color="getTableColor(t.type)"
              class="tablebutton"
              variant="outline"
              @click="selectTable(t)"
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
            <CButton
              class="tablebutton active"
              color="danger"
              variant="outline"
              @click="addDataSetTableModal = true"
            >
              <span>
                <i
                  class="fa fa-plus"
                  aria-hidden="true"
                />
              </span>
              Data Set
            </CButton>
            <CModal
              title="Add Data Set"
              color="danger"
              :show.sync="addDataSetTableModal"
            >
              <template #footer-wrapper>
                <div class="hidden" />
              </template>
              <AddDataSetTable
                :key="index"
                :database="database"
                :dataset="selectedDataset"
                :table-name="selectedTable.tablename"
              />
            </CModal>
          </flex-row>
        </flex-col>
      </div>
    </flex-col>
    <CCollapse
      :show="collapse"
      :duration="300"
      class="mt-2 borderpad"
    >
      <!--
        modal for editing a column (not visible until edit is clicked)
      -->
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
          :table-name="selectedTable.tablename"
          :table-type="selectedTable.type"
          :column-name="itemData.column_name"
          :edit-column-name="itemData.column_name"
          :data-type="itemData.data_type"
          :edit-data-type="itemData.data_type"
          :columns="columns"
          is-edit
        />
      </CModal>
      <!--
        dataset edit area
      -->
      <div
        :id="database + '_dataset'"
        hidden
      >
        {{ showSelectedTable() }}
        <EditDataSetTable
          :key="index"
          :database="database"
          :dataset="selectedDataset"
          :table-name="selectedTable.tablename"
        />
      </div>
      <!--
        main table
      -->
      <div
        :id="database + '_table'"
        hidden
      >
        <CDataTable
          :items="columns"
          :fields="fields"
          table-filter
          columns-per-page-select
          sorter
          pagination
        >
          <template
            #edit="{item}"
            :columns="columns"
          >
            <td>
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
          <template
            #fts="{item}"
          >
            <td>
              <div v-if="item.data_type === 'text' && selectedTable.type ==='search'">
                <label
                  class="checkbox"
                >
                  <input
                    type="checkbox"
                    :checked="item.fts"
                    @click="setFTS(item)"
                  >
                  <span class="checkmark" />
                </label>
              </div>
            </td>
          </template>
          <nav aria-label="pagination">
            <ul class="pagination" />
          </nav>
          <template #under-table="addTable">
            <flex-row class="underTable">
              <div
                v-if="selectedTable.type ==='search'"
                class="margin-right"
              >
                <CDropdown
                  ref=""
                  :toggler-text="selectedDisplayField"
                >
                  <CDropdownItem
                    v-for="c in columns"
                    :key="c.column_name"
                    @click.native="setFTSDisplayField(c.column_name)"
                  >
                    {{ c.column_name }}
                  </CDropdownItem>
                </CDropdown>
              </div>
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
                  :table-name="selectedTable.tablename"
                  :table-type="selectedTable.type"
                  :columns="columns"
                >
                  ADD COLUMN
                </EditTableColumn>
              </CModal>
            </flex-row>
          </template>
        </CDataTable>
      </div>
    </CCollapse>
  </div>
</template>
<script>

import AddTable from './AddTable.vue'
import AddSearchTable from './AddSearchTable.vue'
import AddDataSetTable from './AddDataSetTable.vue'
import EditDataSetTable from './EditDataSetTable.vue'
import EditTableColumn from './EditTableColumn.vue'

export default {
  name: 'DatabaseCard',
  components: {
    AddTable,
    AddSearchTable,
    AddDataSetTable,
    EditDataSetTable,
    EditTableColumn
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
      selectedTable: {},
      selectedDataset: '',
      collapse: false,
      addTableModal: false,
      addSearchTableModal: false,
      addDataSetTableModal: false,
      addTableColumnModal: false,
      editTableColumnModal: false,
      selectedDisplayField: 'select',
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
    formatDatabaseName () {
      const r = /^[0-9]+_/gi
      return this.database.replace(r, '')
    },
    getTableColor (t) {
      if (t === 'search') {
        return 'warning'
      } else if (t === 'dataset') {
        return 'danger'
      } else {
        return 'info'
      }
    },
    editTableColumn (i) {
      this.editTableColumnModal = true
      this.itemData = i
    },
    selectTable (table) {
      if (table.tablename === this.selectedTable.tablename && this.collapse === true) {
        this.collapse = false
        return
      }
      this.selectedTable = table
      if (table.type === 'dataset') {
        this.selectedDataset = table.data
        this.collapse = true
        document.getElementById(this.database + '_dataset').hidden = false
        document.getElementById(this.database + '_table').hidden = true
        // this.addDataSetTableModal = true
      } else {
        this.selectedDataset = ''
        document.getElementById(this.database + '_dataset').hidden = true
        document.getElementById(this.database + '_table').hidden = false
        this.getTableSchema(table.tablename)
      }
    },
    showSelectedTable () {
      if (this.selectedTable) {
        return this.selectedTable.tablename
      }
    },
    setFTS (c) {
      console.log(c)
      if (c.fts === false) {
        c.fts = true
      } else {
        c.fts = false
      }
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/setFTS', {
        params: {
          database: vm.database,
          table: vm.selectedTable.tablename,
          column: c.column_name,
          fts: c.fts
        }
      })
        .then(function (response) {
          if (response.data) {
            for (var i = 0; i < vm.columns; i++) {
              if (vm.columns[i].column_name === c.columns_name) {
                vm.columns[i].fts = c.fts
              }
            }
          }
        })
    },
    setFTSDisplayField (c) {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/setFTSDisplayField', {
        params: {
          database: vm.database,
          table: vm.selectedTable.tablename,
          display_field: c
        }
      })
        .then(function (response) {
          if (response.data) {
          }
        })
    },
    getTableSchema (table) {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/getTableSchema', {
        params: {
          database: vm.database,
          table: table
        }
      })
        .then(function (response) {
          if (response.data) {
            vm.columns = response.data.d
            if (vm.columns.length === 0) {
              vm.collapse = true
              return
            }
            for (var i in vm.columns) {
              if (vm.columns[i].fts === true) {
                vm.selectedDisplayField = vm.columns[i].display_field
                break
              }
            }
            vm.fields = Object.keys(vm.columns[0])
            if (vm.selectedTable.type === 'search') {
              vm.fields.push({
                key: 'fts',
                label: 'Full Text Search',
                sorter: false,
                filter: false
              })
            }
            vm.fields.push({
              key: 'edit',
              label: 'edit',
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
          table: vm.selectedTable.tablename
        }
      })
        .then(function (response) {
          if (response.data) {
            if (response.data.status === 'success') {
              vm.$parent.getDatabases()
              vm.collapse = false
            }
          }
        })
    },
    deleteDB () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/deleteDatabase', {
        params: {
          database: vm.database
        }
      })
        .then(function (response) {
          if (response.data) {
            if (response.data.status === 'success') {
              vm.$parent.getDatabases()
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
    color: #2c3e50;
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
  width: 20px;
  color: #39b2d5;
  background-color: #fff;
}
.tablebutton {
  color: #2c3e50;
  background-color: #efefef;
  margin: 10px;
}
.container {
    padding: 0px;
}
.card {
    min-height: 125px;
    margin-left: 10px;
    margin-right: 10px;
    margin-bottom: 10px;
    margin-top: 20px;
    border: 1px solid #fff;
    border-radius: 5px;
    background-color: #fff;
    -webkit-box-shadow:0 2px 4px 0 #b2b5be;
       -moz-box-shadow:0 2px 4px 0 #b2b5be;
            box-shadow:0 2px 4px 0 #b2b5be;
}
.database {
    min-width: 100px;
    width: 100%;
    border-radius: 3px;
    color: #2c3e50;
}
.borderpad {
  border: 1px solid #c8ced3;
  border-radius: 4px;
  padding: 10px;
  margin: 10px;
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
.underTable {
    float: right;
}
.pointer {
    cursor: pointer;
}
#dataset {
    display: block;
}
.checkbox {
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

.checkbox input {
  position: absolute;
  opacity: 0;
  cursor: pointer;
  height: 0;
  width: 0;
}

.checkmark {
  position: absolute;
  margin-left: auto;
  margin-right: auto;
  height: 20px;
  width: 20px;
  background-color: #eee;
}

.checkbox input:checked ~ .checkmark {
  background-color: #39b2d5;
}

.checkmark:after {
  content: "";
  position: absolute;
  display: none;
}

.checkbox input:checked ~ .checkmark:after {
  display: block;
}

.checkbox .checkmark:after {
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
