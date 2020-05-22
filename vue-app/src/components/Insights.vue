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
    </flex-row>
    <div class="results">
      <!--
      <li
        v-for="item in results"
        :key="item.url"
      >
        {{ item.url }}
      </li>
      -->
      <ResultCard
        v-for="item in results"
        :key="item.url"
        :url="item.url"
        :title="item.title"
        :snippet="item.snippet"
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
      error: '',
      tables: [],
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
      console.log('this.results a')
      console.log(this.results)
      this.results = []
      console.log('this.results b')
      console.log(this.results)
      var vm = this

      this.$axios.get(this.$SERVER_URI + '/search', {
        params: {
          query: vm.query,
          database: vm.selectedDatabase,
          table: vm.selectedTable,
          lang: 'en'
        }
      })
        .then(function (response) {
          console.log('c')
          console.log(response)
          if (response.data) {
            console.log('d')
            if (response.data.items) {
              vm.hasError = false
              vm.results = response.data.items
              console.log('this.results c')
              console.log(vm.results)
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
    width: 60%;
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
