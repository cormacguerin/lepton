<template>
  <div>
    <flex-col
      no-wrap
      class="container"
    >
      <div class="chart">
        <flex-row
          class="left"
        >
          <div class="dropdown">
            <CDropdown
              ref="databaseDropDown"
              :toggler-text="selectedDatabase"
              color="bg-dark"
              no-caret
              nav
              placement="start"
            >
              <CDropdownItem
                v-for="d in dbs"
                :key="d.key"
                @click="selectDatabase(d.key,d.tables)"
              >
                {{ formatDatabaseName(d.key) }}
              </CDropdownItem>
            </CDropdown>
          </div>
          <div class="dropdown">
            <CDropdown
              ref="DatasetDropDown"
              :toggler-text="selectedDataset"
              color="bg-dark"
              no-caret
              nav
              placement="start"
            >
              <CDropdownItem
                v-for="t in tables"
                :key="t.tablename"
                @click="selectDataset(t)"
              >
                {{ t.tablename }}
              </CDropdownItem>
            </CDropdown>
          </div>
          <div class="dropdown">
            <CDropdown
              ref="columnDropDown"
              :toggler-text="selectedColumn"
              color="bg-dark"
              no-caret
              nav
              placement="start"
            >
              <CDropdownItem
                v-for="c in columns"
                :key="c"
                @click="selectColumn(c.column_name)"
              >
                {{ c.column_name }}
              </CDropdownItem>
            </CDropdown>
          </div>
          <div class="dropdown">
            <CDropdown
              ref="dimensionsDropDown"
              :toggler-text="selectedDimension"
              color="bg-dark"
              no-caret
              nav
              placement="start"
            >
              <CDropdownItem
                v-for="d in dimensions"
                :key="d"
                @click="selectDimension(d)"
              >
                {{ d }}
              </CDropdownItem>
            </CDropdown>
          </div>
          <CButton
            color="success"
            class="tablebutton active"
            @click="addDataset"
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
        </flex-row>
        <!--
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
          </flex-row>
        </flex-col>
        -->
        <line-chart :chart-data="datacollection"></line-chart>
        <button @click="fillData()">Randomize</button>
      </div>
    </flex-col>
  </div>
</template>
<script>

import LineChart from './charts/LineChart.js'

export default {
  name: 'EditChart',
  components: {
    LineChart
  },
  props: {
    dbs: {
      type: Object,
      default: function () {
        return { }
      }
    },
    datasets: {
      type: Array,
      default: function () {
        return []
      }
    }
  },
  data () {
    return {
      selectedDatabase: 'database',
      selectedDataset: 'dataset',
      selectedColumn: 'column',
      datacollection: null,
      dimensions: ['X', 'Y'],
      query: '',
      tables: [],
      columns: [],
      chartdatasets: []
    }
  },
  created () {
  },
  mounted () {
  //  this.fillData()
  },
  methods: {
    fillData () {
      this.datacollection = {
        labels: ['A'],
        datasets: [
          {
            label: 'Data One',
            backgroundColor: '#f87979',
            data: [this.getRandomInt(), this.getRandomInt(), this.getRandomInt(), this.getRandomInt()]
          }, {
            label: 'Data One',
            backgroundColor: '#000',
            data: [this.getRandomInt(), this.getRandomInt(), this.getRandomInt(), this.getRandomInt()]
          }
        ]
      }
    },
    getRandomInt () {
      return Math.floor(Math.random() * (50 - 5 + 1)) + 5
    },
    formatDatabaseName (database) {
      console.log(database)
      const r = /^[0-9]+_/gi
      return database.replace(r, '')
    },
    selectDatabase (database, tables) {
      this.selectedDatabase = database
      this.tables = []
      for (var i in tables) {
        if (tables[i].type === 'dataset') {
          this.tables.push(tables[i])
        }
      }
      this.$refs.databaseDropDown.hide()
    },
    selectDataset (ds) {
      var dataset = JSON.parse(ds.data)
      console.log(dataset)
      this.query = dataset.query
      this.fields = dataset.fields
      this.selectedDataset = ds.tablename
      this.$refs.DatasetDropDown.hide()
    },
    selectColumn (column) {
      this.selectedColumn = column
      this.$refs.columnDropDown.hide()
    },
    /*
    getTableSchema (table) {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/getTableSchema', {
        params: {
          database: vm.selectedDatabase,
          table: table
        }
      })
        .then(function (response) {
          if (response.data) {
            vm.selectedTable = table
            vm.columns = response.data.d
            vm.$refs.DatasetDropDown.hide()
          }
        })
        .catch(function (error) {
          console.log(error)
        })
    },
    */
    selectDimension (d) {
      this.selectedDimension = d
    },
    addDataset (d) {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/runQuery', {
        params: {
          database: vm.selectedDatabase,
          query: vm.query
        }
      })
        .then(function (response) {
          if (response.data) {
            if (response.data.status === 'success') {
              console.log(response.data.message)
              var labels = []
              var datasetsObj = {}
              // one of the keys should be a label, remove it
              for (var i in response.data.message[0]) {
                var dataset = {}
                dataset.data = []
                dataset.fill = false
                dataset.label = i
                dataset.backgroundColor = '#afafaf'
                datasetsObj[i] = dataset
              }
              for (var j in response.data.message) {
                console.log(j)
                console.log(response.data.message[j])
                labels.push(response.data.message[j].customer_name)
                for (var k in response.data.message[j]) {
                  console.log('k ' + k)
                  console.log(response.data.message[j][k])
                  datasetsObj[k].data.push(response.data.message[j][k])
                }
              }
              console.log('datasetsObj')
              console.log(datasetsObj)
              console.log(Object.values(datasetsObj))
              vm.datacollection = {
                labels: labels,
                datasets: Object.values(datasetsObj)
              }
              console.log('vm.datacollection')
              console.log(vm.datacollection)
            } else {
              console.log('failed')
              console.log(response.data.error)
            }
            /*
            datasets: [
              {
                label: 'Data One',
                backgroundColor: '#f87979',
                data: [this.getRandomInt(), this.getRandomInt(), this.getRandomInt(), this.getRandomInt()]
              }, {
                label: 'Data One',
                backgroundColor: '#000',
                data: [this.getRandomInt(), this.getRandomInt(), this.getRandomInt(), this.getRandomInt()]
              }
            ]
            */
          }
        })
    }
  }
}
</script>

<style scoped>
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
  margin: 5px;
}
.container {
    padding: 0px;
}
.chart {
}
.dropdown {
  margin: 5px;
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
