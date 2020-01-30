<template>
  <div class="card">
    <flex-row
      no-wrap
      class="container"
    >
      <div class="database">
        <flex-col
          class="left"
        >
          <h2>{{ database }}</h2>
          <div
            class="database-icon"
          >
            <div class="cylinder" />
            <div class="cylinder" />
            <div class="cylinder" />
          </div>
        </flex-col>
      </div>
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
            :key="t.tables"
            color="info"
            class="tablebutton"
            variant="outline"
            @click="getTableSchema(t.tables)"
          >
            {{ t.tables }}
          </CButton>
          <CButton
            color="info"
            class="tablebutton active"
            @click="createTableModal = true"
          >
            <span>
              <i
                class="fa
                fa-plus"
                aria-hidden="true"
              />
            </span>
          </CButton>
          <CModal
            title="Add Table"
            color="info"
            :show.sync="createTableModal"
          >
            <template #footer-wrapper>
              <div class="hidden" />
            </template>
            <CreateTable
              :key="index"
              :database="database"
            />
          </CModal>
        </flex-row>
      </flex-col>
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
    <CCollapse
      :show="collapse"
      :duration="300"
      class="mt-2"
    >
      <CDataTable
        :items="items"
        :fields="fields"
        table-filter
        items-per-page-select
        sorter
        pagination
      >
        <template #edit="{item, index}">
          <td class="py-2">
            <CButton
              color="primary"
              variant="outline"
              square
              size="sm"
              @click="editTable(item)"
            >
              Edit
            </CButton>
            <CModal
              title="Edit Table"
              color="info"
              :show.sync="editTableModal"
            >
              <template #footer-wrapper>
                <div class="hidden" />
              </template>
              <EditTable
                :key="index"
                :database="database"
                :table-name="item"
              />
            </CModal>
          </td>
        </template>
        <nav aria-label="pagination">
          <ul class="pagination" />
        </nav>
        <template #under-table="addTable">
          <div class="addTable">
            <CButton
              class="btn active"
              color="info"
              @click="addTableModal = true"
            >
              <span>
                Add Column
                <i
                  class="fa fa-plus"
                  aria-hidden="true"
                />
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
              <EditTable
                :key="index"
                :database="database"
                :table-name="selected"
              >
                ADD COLUMN
              </EditTable>
            </CModal>
          </div>
        </template>
      </CDataTable>
    </CCollapse>
  </div>
</template>
<script>

import CreateTable from './CreateTable.vue'
import EditTable from './EditTable.vue'

export default {
  name: 'DatabaseCard',
  components: {
    CreateTable,
    EditTable
  },
  props: {
    database: {
      type: String,
      default: function () {
        return { }
      }
    },
    tables: {
      type: Object,
      default: function () {
        return { }
      }
    }
  },
  data () {
    return {
      items: [
      ],
      fields: [
      ],
      details: [
      ],
      selected: '',
      collapse: false,
      createTableModal: false,
      addTableModal: false,
      editTableModal: false
    }
  },
  created () {
  },
  methods: {
    editTable (item) {
      this.editTableModal = true
    },
    getTableSchema (table, persist) {
      if (this.selected === table && persist !== true) {
        this.collapse = false
        this.selected = ''
        return
      }
      var vm = this
      this.$axios.get('https://35.239.29.200/api/getTableSchema', {
        params: {
          database: vm.database,
          table: table
        }
      })
        .then(function (response) {
          if (response.data) {
            vm.items = response.data.d
            vm.fields = Object.keys(vm.items[0])
            vm.fields.push({
              key: 'edit',
              label: '',
              _style: 'width:1%',
              sorter: false,
              filter: false
            })
            vm.selected = table
            vm.collapse = true
          }
        })
        .catch(function (error) {
          console.log(error)
        })
    },
    deleteDB () {
      var vm = this
      this.$axios.get('https://35.239.29.200/api/deleteDatabase', {
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
    padding-top: 15px;
    color: #004c65;
    text-align: center;
    font-size: 1.2em;
}
.database-icon {
    transform: rotate(180deg);
    width: 100%;
}
.cylinder {
    margin-top: -5px;
    width: 100%;
    height: 20px;
    border-top: 2px solid #4e97ad;
    border-bottom: 1px solid #efefef;
    border-radius: 50%;
    background-color: #fff;
    -webkit-box-shadow:0 -4px 4px 0px rgba(89, 183, 187, 0.3), 0 0 0px rgba(0, 0, 0, 0.1) inset;
       -moz-box-shadow:0 -4px 4px 0px rgba(89, 183, 187, 0.3), 0 0 0px rgba(0, 0, 0, 0.1) inset;
            box-shadow:0 -4px 4px 0px rgba(89, 183, 187, 0.3), 0 0 0px rgba(0, 0, 0, 0.1) inset;
}
.buttons {
  margin-left: 10px;
  margin-right: 10px;
}
.tablebutton {
  margin: 10px;
}
.container {
    padding: 0px;
}
.card {
    min-height: 125px;
    margin: 10px;
    padding: 10px;
    border: 1px solid #fff;
    border-radius: 3px;
    background-color: #fff;
    box-shadow: 0px 1px 5px rgba(0,0,0,0.3), 0 0px 0px rgba(0,0,0,0.22);
}
.database {
    min-width: 100px;
}
.left {
    min-width: 100px;
}
.edit {
  margin-right: 5px;
  margin-left: 5px;
  width: 10px;
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
