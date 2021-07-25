<template>
  <div class="container">
    <flex-row>
      <div
        id="searchBar"
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
      <div
        id="ecsuggestbox"
        v-on-clickaway="hideSuggest"
      >
        <div
          v-for="suggestion in suggestions"
          :key="suggestion"
          class="suggestion"
          @click="choseSuggestion(suggestion)"
        >
          {{ suggestion }}
        </div>
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
    <flex-row>
      <CDropdown
        v-for="(valueArray, key) in metadata"
        ref=""
        :key="key"
        :toggler-text="key"
        class="filterbutton"
      >
        <CDropdownItem
          v-for="v in valueArray"
          :key="v"
          class="dropdownstyle"
          @click.native="filterMeta(key,v)"
        >
          {{ v }}
        </CDropdownItem>
      </CDropdown>
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
    <flex-row>
      <div
        v-for="(filter, index) in metaFilter"
        :key="index"
        class="filterbutton"
      >
        <flex-row
          class="active metaFilter"
        >
          <div>
            {{ filter.key }}
            {{ filter.value }}
          </div>
          <div
            class="removeFilter"
            @click="removeFilter(index)"
          >
            | X
          </div>
        </flex-row>
      </div>
    </flex-row>
    <div class="results">
      <ResultCard
        v-for="item in results"
        :key="item.url"
        :url="item.url"
        :title="item.data.title"
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
    <flex-row class="pagination">
      <div
        v-for="p in resultPages"
        :key="p"
        class="page"
        @click="selectPage(p,true)"
      >
        {{ p }}
      </div>
    </flex-row>
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
      suggestions: [],
      keySelectSuggest: -1,
      hasResults: true,
      hasError: false,
      inFlight: false,
      pageNumber: 0,
      resultPages: [],
      fullMeta: {},
      metadata: {},
      metaFilter: [],
      error: '',
      tables: [],
      operators: ['equals', 'contains', 'less_than', 'greater_than'],
      filters: [],
      dbs: {}
    }
  },
  created () {
    this.getDatabases()
  },
  methods: {
    search () {
      this.results = []
      var vm = this

      // seems to go crazy(403) when we run suggest and seach at the same time, need to investigate
      this.inFlight = true
      setTimeout(function () { vm.inFlight = false }, 300)

      var filter = JSON.stringify(this.metaFilter)
      var pages = JSON.stringify({
        page_number: this.pageNumber,
        page_result_count: 20
      })

      this.$axios.get(this.$SERVER_URI + '/search', {
        params: {
          query: vm.query,
          database: vm.selectedDatabase,
          table: vm.selectedTable,
          filter: filter,
          pages: pages,
          lang: 'en'
        }
      })
        .then(function (response) {
          if (response.data) {
            if (response.data.items) {
              vm.hasError = false
              vm.results = response.data.items

              for (var r = 0; r < vm.results.length; r++) {
                var meta = {}
                try {
                  meta = JSON.parse(vm.results[r].data.metadata)
                } catch (e) {
                  console.log(e)
                }
                for (var key in meta) {
                  if (!(key in vm.metadata)) {
                    vm.metadata[key] = []
                  }
                  var found = false
                  for (var i = 0; i < vm.metadata[key].length; i++) {
                    if (vm.metadata[key][i] === meta[key]) {
                      found = true
                    }
                  }
                  if (found === false) {
                    vm.metadata[key].push(meta[key])
                  }
                }
              }
              for (var key_ in vm.metadata) {
                if (vm.metadata[key_].length < 2) {
                  delete vm.metadata[key_]
                }
              }

              if (vm.results.length > 0) {
                vm.hasResults = true
                for (var i_ = 1; i_ <= (response.data.result_count / 21); i_++) {
                  vm.resultPages.push(i_)
                  if (vm.pageNumber < 5) {
                    if (i_ > 10) {
                      break
                    }
                  } else {
                    if (i_ > vm.pageNumber + 5) {
                      vm.resultPages.splice(0, vm.pageNumber - 5)
                      break
                    }
                  }
                }
              } else {
                vm.hasResults = false
              }

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
    suggest () {
      if (this.inFlight === true) {
        return
      }
      var vm = this
      if (!this.query) {
        return
      }

      this.$axios.get(this.$SERVER_URI + '/search', {
        params: {
          query: vm.query,
          corpus: 'ecommerce'
        }
      })
        .then(function (response) {
          if (response.data) {
            if (response.data.suggestions) {
              vm.suggestions = []
              for (var i in response.data.suggestions) {
                vm.suggestions.push(response.data.suggestions[i].split(':').join(' '))
              }
              // todo CJK replace
              if (vm.suggestions.length > 0) {
                document.getElementById('ecsuggestbox').style.display = 'inline-block'
              } else {
                document.getElementById('ecsuggestbox').style.display = 'none'
              }
            }
          }
        })
    },
    hideSuggest () {
      document.getElementById('ecsuggestbox').style.display = 'none'
    },
    selectPage (p, s) {
      this.pageNumber = p - 1
      this.search()
      var searchBar = document.getElementById('searchBar')
      if (s) {
        searchBar.scrollIntoView({ behavior: 'smooth' })
      }
    },
    choseSuggestion (s) {
      this.query = s
      this.pageNumber = 0
      this.search()
    },
    filterCategory (v) {
      if (v) {
        if (v === 'article') {
          this.selectedCategory = 'Articles'
        }
        if (v === 'section') {
          this.selectedCategory = 'Sections'
        }
        this.categoryFilter = { key: 'type', value: v, operator: 'equals' }
      } else {
        this.selectedCategory = 'Category'
        this.categoryFilter = []
      }
      this.pageNumber = 0
      this.search()
    },
    filterMeta (k, v) {
      if (k && v) {
        this.metaFilter.push({ key: k, value: v, operator: 'equals' })
      }
      this.pageNumber = 0
      this.search()
    },
    removeFilter (i) {
      this.metaFilter.splice(i, 1)
      this.search()
    },
    processKeys (e) {
      const sbcn = document.getElementById('ecsuggestbox').childNodes
      if (e.keyCode === 40) {
        this.keySelectSuggest++
      } else if (e.keyCode === 38) {
        if (this.keySelectSuggest > 0) {
          this.keySelectSuggest--
        }
      } else if (e.keyCode === 13) {
        if (this.keySelectSuggest >= 0) {
          this.choseSuggestion(this.suggestions[this.keySelectSuggest])
        } else {
          this.pageNumber = 0
          this.search()
        }
      } else {
        return
      }
      for (var i = 0; i < sbcn.length; i++) {
        sbcn[i].style.backgroundColor = 'white'
        sbcn[i].style.color = 'black'
      }
      if (this.keySelectSuggest < sbcn.length) {
        if (sbcn[this.keySelectSuggest]) {
          sbcn[this.keySelectSuggest].style.backgroundColor = '#fafafa'
          sbcn[this.keySelectSuggest].style.color = '#aaa'
        }
      } else {
        this.keySelectSuggest = -1
      }
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
.filterbutton {
    margin: 20px;
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
.page {
    cursor: pointer;
    margin: 20px;
    padding: 7px;
    border: 1px solid #ccc;
    border-radius: 5px;
}
.metaFilter {
    margin: 10px;
    padding: 10px;
    background-color: #d0af00;
    color: white;
}
</style>
