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
              {{ scopes.includes(index) ? 'Hide' : 'Show' }}
            </div>
          </td>
        </template>
        <template #details="{item, index}">
          <CCollapse
            :show="scopes.includes(index)"
          >
            <flex-row class="scopeContainer">
              <div class="scopes">
                <CButton
                  size="sm"
                  color="info"
                  class=""
                >
                  scope info here
                </CButton>
                <CButton
                  size="sm"
                  color="danger"
                  class="ml-1"
                >
                  Delete
                </CButton>
              </div>
              <div>
                <CButton
                  class="squarebtn active"
                  color="info"
                  size="sm"
                  @click="addApiScopeModal = true"
                >
                  <i
                    class="fa fa-plus"
                    aria-hidden="true"
                  />
                </CButton>
              </div>
            </flex-row>
          </CCollapse>
        </template>
        <template
          #delete="{item}"
        >
          <td>
            <div class="action">
              delete
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
  name: 'Configure',
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
    getApiKeys () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/getApiKeys', {
      })
        .then(function (response) {
          if (response.data.status === 'success') {
            vm.columns = response.data.message
          }
        })
    },
    toggleScope (index) {
      const position = this.scopes.indexOf(index)
      position !== -1 ? this.scopes.splice(position, 1) : this.scopes.push(index)
      this.selectedKey = this.columns[index].id
      console.log('this.selectedKey')
      console.log(this.selectedKey)
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
.squarebtn {
    width: 30px;
    height: 30px;
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
