<template>
  <div class="container">
    <flex-row>
      <div
        class="searchbox"
        nowrap
      >
        <input
          v-model="query"
          class="search"
          placeholder=""
        >
      </div>
      <div
        class="search-margin"
      >
        <CButton
          class="active"
          color="info"
          @click="search"
        >
          Search
        </CButton>
      </div>
      <flex-row class="search-margin">
        <div class="dropdown">
          <CDropdown
            ref="databaseDropDown"
            :toggler-text="selectedDatabase"
            title="database"
          >
            <CDropdownItem
              v-for="d in dbs"
              :key="d.key"
              @click.native="selectDatabase(d)"
            >
              {{ d.key }}
            </CDropdownItem>
          </CDropdown>
        </div>
        <div class="dropdown">
          <CDropdown
            ref="tableDropDown"
            :toggler-text="selectedTable"
            title="table (optional)"
          >
            <CDropdownItem
              v-for="t in tables"
              :key="t"
              @click.native="selectTable(t)"
            >
              {{ t }}
            </CDropdownItem>
          </CDropdown>
        </div>
      </flex-row>
      <flex-row>
        <div
          class="search-margin"
        >
          <CButton
            color="success"
            class="addfilterbutton active"
            @click="addFilterModal = true"
          >
            <span>
              <i
                class="fa
                fa-plus"
                aria-hidden="true"
              />
              Filter
            </span>
          </CButton>
        </div>
        <CModal
          title="Add Metadata Filter"
          color="info"
          :show.sync="addFilterModal"
        >
          <flex-col
            justify="start"
          >
            <div
              class="filterbox"
              nowrap
            >
              <input
                v-model="filterKey"
                class="filter"
                placeholder=""
              >
            </div>
            <div
              class="filterdropdown"
              justify="start"
            >
              <CDropdown
                ref="tableDropDown"
                :toggler-text="selectedOperator"
                title="operator"
              >
                <CDropdownItem
                  v-for="o in operators"
                  :key="o"
                  @click.native="selectOperator(o)"
                >
                  {{ o }}
                </CDropdownItem>
              </CDropdown>
            </div>
            <div
              class="filterbox"
              nowrap
            >
              <input
                v-model="filterValue"
                class="filter"
                placeholder=""
              >
            </div>
            <CButton
              color="info"
              class="addfilter active"
              @click="addFilter"
            >
              Add
            </CButton>
          </flex-col>
          <template #footer-wrapper>
            <div class="hidden" />
          </template>
        </CModal>
      </flex-row>
    </flex-row>
    <flex-row>
      <div
        v-for="f in filters"
        :key="f.key"
        class="filtercontainer"
      >
        <flex-row>
          <div
            class="filter"
          >
            {{ f.key }} : {{ f.operator }} : {{ f.value }}
          </div>
          <div
            class="deletefilter"
          />
        </flex-row>
      </div>
    </flex-row>
    <div class="results">
      <ResultCard
        v-for="item in results"
        :key="item.url"
        :url="item.url"
        :title="item.title"
        :snippet="item.snippet"
        :weight="item.weight"
        :tf="item.tf"
        :tdscore="item.tdscore"
        :docscore="item.docscore"
        :wscore="item.wscore"
        :score="item.score"
        @click.native="open(item)"
      />
    </div>
    <template v-if="hasResults === false">
      <div class="hasresults">
        No Results Found.
      </div>
    </template>
    <template v-if="hasError === true">
      <div class="hasresults">
        Error Obtaining Results.
        <div class="error">
          {{ error }}
        </div>
      </div>
    </template>
  </div>
</template>
<script>

import ResultCard from './ResultCard.vue'

export default {
  name: 'Insights',
  components: {
    ResultCard
  },
  props: {
  },
  data () {
    return {
      query: '',
      results: [],
      selectedDatabase: 'select',
      selectedTable: 'select',
      selectedOperator: 'equals',
      addFilterModal: false,
      error: '',
      tables: [],
      operators: ['equals', 'contains', 'less_than', 'greater_than'],
      filters: [],
      dbs: {},
      hasResults: true,
      hasError: false
    }
  },
  created () {
    this.getDatabases()
  },
  methods: {
    search () {
      this.results = []
      var vm = this

      this.$axios.get(this.$SERVER_URI + '/search', {
        params: {
          query: vm.query,
          database: vm.selectedDatabase,
          table: vm.selectedTable,
          filter: JSON.stringify(vm.filters),
          lang: 'en'
        }
      })
        .then(function (response) {
          if (response.data) {
            console.log('d')
            if (response.data.items) {
              vm.hasError = false
              vm.results = response.data.items
              if (vm.results.length > 0) {
                vm.hasResults = true
              } else {
                vm.hasResults = false
              }
            } else {
              vm.hasError = true
            }
          }
        })
    },
    open (l) {
      console.log(l)
    },
    addFilter () {
      if (this.filterKey && this.filterValue && this.selectedOperator) {
        var filter = {
          key: this.filterKey,
          value: this.filterValue,
          operator: this.selectedOperator
        }
      }
      this.filters.push(filter)
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
    },
    selectDatabase (d) {
      this.tables = []
      if (d.key) {
        this.selectedDatabase = d.key
        for (var t in d.tables) {
          this.tables.push(d.tables[t].tablename)
        }
      }
    },
    selectTable (t) {
      if (t) {
        this.selectedTable = t
      }
    },
    selectOperator (o) {
      if (o) {
        this.selectedOperator = o
      }
    }
  }
}

</script>
<style scoped>
.container {
    width: 80%;
    max-width: 1000px;
    margin-top: 10px;
    margin-left: auto;
    margin-right: auto;
}
.dropdown {
    margin-left: 10px;
    margin-right: 10px;
}
.margin-left {
    margin-left: 10px;
}
.searchbox {
    width: 50%;
    margin: 20px;
}
input.search {
    height: 35px;
    width: 100%;
    padding-left: 10px;
    border: 1px #efefef solid;
    border-radius: 5px;
    background: white;
}
.filtercontainer {
    padding: 5px;
    margin-left: 20px;
    margin-top: 5px;
    margin-bottom: 5px;
    border: 1px solid #efefef;
    border-radius: 3px;
}
.filterdropdown {
    margin: 20px;
    width: 100px;
}
.filterbox {
    width: 200px;
    margin: 10px;
}
.addfilter {
    margin: 20px;
    width: 100px;
}
.hasresults {
    font-size: 2em;
    color: #efefef;
    font-weight: bold;
}
.search-margin {
    margin-left: 10px;
    margin-top: 20px;
}
h2 {
    padding: 15px 0px 0px 0px;
    margin: 0px;
    text-align: center;
    font-size: 24px;
}
</style>
