<template>
  <div class="container">
    <flex-row
      justify="center"
    >
      <div class="flexgrow" />
      <div class="addarea">
        <CButton
          class="btn active"
          color="info"
          @click="addApiKeyModal = true"
        >
          <span>
            <i
              class="fa fa-plus"
              aria-hidden="true"
            />
            API Key
          </span>
        </CButton>
        <CModal
          title="Add API key"
          color="info"
          :show.sync="addApiKeyModal"
        >
          <template #footer-wrapper>
            <div class="hidden" />
          </template>
          <AddApiKey
            :is-open="addApiKeyModal"
          />
        </CModal>
      </div>
    </flex-row>
    <div class="tableContainer">
      <CDataTable
        :items="columns"
        :fields="fields"
        table-filter
        items-per-page-select
        sorter
        hover
        pagination
      >
        <template #show_details="{item, index}">
          <td>
            <div
              class="action"
              @click="toggleScope(index)"
            >
              {{ scopes.includes(index) ? 'Close' : 'Open' }}
            </div>
          </td>
        </template>
        <template #details="{item, index}">
          <CCollapse
            :show="scopes.includes(index)"
          >
            <flex-row class="scopeContainer">
              <div
                v-for="scope in item.scope"
                :key="scope"
                class="scope"
              >
                <flex-row>
                  <div class="scopeApi">
                    {{ scope.api }}
                  </div>
                  <div class="scopeDatabase">
                    {{ scope.database }}
                  </div>
                  <div
                    v-if="scope.table"
                    class="scopeTable"
                  >
                    {{ scope.table }}
                  </div>
                  <div
                    class="scopeDelete"
                    @click="deleteApiScope(item.id, scope)"
                  >
                    <i
                      class="fa fa-minus"
                      aria-hidden="true"
                    />
                  </div>
                </flex-row>
              </div>
              <div>
                <CButton
                  class="active addScope"
                  color="info"
                  size="sm"
                  @click="addApiScopeModal = true"
                >
                  <i
                    class="fa fa-plus"
                    aria-hidden="true"
                  />
                  Add Scope
                </CButton>
              </div>
            </flex-row>
          </CCollapse>
        </template>
        <template
          #delete="{item}"
        >
          <td>
            <div
              class="action"
              @click="deleteApiKey(item.id)"
            >
              Delete
            </div>
          </td>
        </template>
        <nav aria-label="pagination">
          <ul class="pagination" />
        </nav>
      </CDataTable>
      <CModal
        title="Add API Scope"
        color="info"
        :show.sync="addApiScopeModal"
      >
        <template #footer-wrapper>
          <div class="hidden" />
        </template>
        <AddApiScope
          :is-open="addApiScopeModal"
          :api-key="selectedKey"
          :dbs="dbs"
        />
      </CModal>
    </div>
  </div>
</template>
<script>

import AddApiKey from './AddApiKey.vue'
import AddApiScope from './AddApiScope.vue'

export default {
  name: 'ApiKeys',
  components: {
    AddApiKey,
    AddApiScope
  },
  data () {
    return {
      dbs: {},
      addApiKeyModal: false,
      addApiScopeModal: false,
      columns: [],
      fields: [
        {
          key: 'id',
          _style: 'width:15%'
        },
        {
          key: 'name',
          _style: 'width:40%'
        },
        {
          key: 'key',
          _style: 'width:15%'
        },
        {
          key: 'show_details',
          _style: 'width:15%',
          sorter: false,
          filter: false,
          label: 'scope'
        },
        {
          key: 'delete',
          _style: 'width:15%',
          sorter: false,
          filter: false,
          label: ''
        }
      ],
      scopes: [],
      selectedKey: ''
    }
  },
  created () {
    this.getApiKeys()
    this.getDatabases()
  },
  methods: {
    deleteApiKey (id) {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/deleteApiKey', {
        params: {
          key_id: id
        }
      })
        .then(function (response) {
          if (response.data.status === 'success') {
            vm.getApiKeys()
          }
        })
    },
    deleteApiScope (id, scope) {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/deleteApiScope', {
        params: {
          key_id: id,
          api_scope: scope.api,
          api_database: scope.database,
          api_table: scope.table
        }
      })
        .then(function (response) {
          if (response.data.status === 'success') {
            vm.getApiKeys()
          }
        })
    },
    getApiKeys () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/getApiKeys', {
      })
        .then(function (response) {
          if (response.data.status === 'success') {
            vm.columns = response.data.message
            console.log(vm.columns)
          }
        })
    },
    toggleScope (index) {
      const position = this.scopes.indexOf(index)
      position !== -1 ? this.scopes.splice(position, 1) : this.scopes.push(index)
      this.selectedKey = this.columns[index].id
    },
    getDatabases () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/getDatabases', {
      })
        .then(function (response) {
          if (response.data) {
            vm.dbs = response.data
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
.flexgrow {
    flex-grow: 2;
}
.container {
    width: 80%;
    max-width: 1000px;
    margin-left: auto;
    margin-right: auto;
}
.scope {
  margin: 10px;
  border-radius: 3px;
}
.addScope {
  margin: 10px;
}
.scopeApi {
  font-weight: bold;
  padding: 3px;
  background-color: #5f5f5f;
  color: white;
  border-radius: 5px 0px 0px 5px;
}
.scopeDatabase {
  font-weight: bold;
  padding: 3px;
  background-color: #fff;
  color: #5f5f5f;
}
.scopeTable {
  padding: 3px;
  background-color: #fff;
  color: #5f5f5f;
}
.scopeDelete {
  color: white;
  padding: 3px;
  background-color: #f86c6b;
  cursor: pointer;
  border-radius: 0px 5px 5px 0px;
}
.scopeContainer {
  min-height: 50px;
}
.tableContainer {
    margin: 10px;
    padding: 10px;
    border: 1px solid #efefef;
    border-radius: 5px;
}
.action {
    color: #39b2d5;
    text-decoration: underline;
    cursor: pointer;
}
h2 {
    padding: 15px 0px 0px 0px;
    margin: 0px;
    text-align: center;
    font-size: 24px;
}
.addarea {
    margin-top: 50px;
    margin-right: 10px;
}
</style>
