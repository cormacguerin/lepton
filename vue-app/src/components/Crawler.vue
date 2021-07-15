<template>
  <div class="container">
    <flex-row>
      <div
        class="searchbox"
        nowrap
      >
        <input
          v-model="url"
          class="search"
          placeholder=""
        >
      </div>
      <div
        class="search-margin"
      >
        <CButton
          class="active"
          color="warning"
          @click="addUrl"
        >
          <span>
            <i
              class="fa
              fa-plus"
              aria-hidden="true"
            />
            Add
          </span>
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
    <div class="items">
      <flex-row
        v-for="item in items"
        :key="item.id"
        class="itemrow"
      >
        <div
          class="item"
        >
          {{ item.url }}
        </div>
        <div
          class="item"
        >
          {{ item.database }}
        </div>
        <div
          class="item"
        >
          {{ item.tablename }}
        </div>
        <div
          class="delete"
          @click="deleteUrl(item)"
        >
          X
        </div>
      </flex-row>
    </div>
    <template v-if="hasResults === false">
      <div class="hasitems">
        No Results Found.
      </div>
    </template>
    <template v-if="hasError === true">
      <div class="hasitems">
        Error Obtaining Results.
        <div class="error">
          {{ error }}
        </div>
      </div>
    </template>
  </div>
</template>
<script>

export default {
  name: 'Crawler',
  components: {
  },
  props: {
  },
  data () {
    return {
      url: '',
      items: [],
      tables: [],
      dbs: {}
    }
  },
  created () {
    this.getDatabases()
    this.getCrawlerUrls()
  },
  methods: {
    addUrl () {
      var vm = this

      this.$axios.get(this.$SERVER_URI + '/api/addCrawlerUrl', {
        params: {
          url: vm.url,
          database: vm.selectedDatabase,
          table: vm.selectedTable
        }
      })
        .then(function (response) {
          if (response.data) {
            console.log(response.data)
            if (response.data.status === 'success') {
              vm.getCrawlerUrls()
            }
          }
        })
    },
    deleteUrl (item) {
      var vm = this

      this.$axios.get(this.$SERVER_URI + '/api/deleteCrawlerUrl', {
        params: {
          url: item.url,
          database: item.database,
          table: item.tablename
        }
      })
        .then(function (response) {
          if (response.data) {
            if (response.data.status === 'success') {
              vm.getCrawlerUrls()
            }
          }
        })
    },
    getCrawlerUrls () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/getCrawlerUrls', {
      })
        .then(function (response) {
          if (response.data) {
            vm.items = response.data
          }
        })
        .catch(function (error) {
          console.log(error)
        })
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
.items {
    margin: 20px;
}
.item {
    margin: 10px;
    padding: 10px;
    background-color: #fafafa;
}
.delete {
    pointer: cursor;
    padding: 10px;
    padding-top: 20px;
    font-weight: bold;
    background-color: #f86c6b!important;
    color: white;
}
.itemrow {
    justify-content: space-between;
    border: 1px solid #efefef;
    border-radius: 5px;
}
.hasitems {
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
