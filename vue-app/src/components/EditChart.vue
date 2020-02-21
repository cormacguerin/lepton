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
              color="dark"
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
            <div class="dropdownLabel">
              Database
            </div>
          </div>
          <div class="dropdown">
            <CDropdown
              ref="DataSetDropDown"
              :toggler-text="selectedDataSet"
              color="dark"
              no-caret
              nav
              placement="start"
            >
              <CDropdownItem
                v-for="t in tables"
                :key="t.tablename"
                @click="selectDataSet(t)"
              >
                {{ t.tablename }}
              </CDropdownItem>
            </CDropdown>
            <div class="dropdownLabel">
              Data Set
            </div>
          </div>
        </flex-row>
        <flex-row
          class="left"
        >
          <!--
            Chart Selector
          -->
          <div class="dropdown">
            <CDropdown
              ref="chartDropDown"
              :toggler-text="selectedChart"
              color="dark"
              no-caret
              nav
              placement="start"
            >
              <CDropdownItem
                v-for="c in charts"
                :key="c"
                @click="selectChart(c)"
              >
                {{ c }}
              </CDropdownItem>
            </CDropdown>
            <div class="dropdownLabel">
              Chart
            </div>
          </div>
          <!--
            Labels (X) label
          -->
          <div class="dropdown">
            <CDropdown
              ref="labelDropDown"
              :toggler-text="selectedLabel"
              color="dark"
              no-caret
              nav
              placement="start"
            >
              <CDropdownItem
                v-for="d in fields"
                :key="d"
                @click="selectLabel(d)"
              >
                {{ d }}
              </CDropdownItem>
            </CDropdown>
            <div class="dropdownLabel">
              Label(X-axis)
            </div>
          </div>
          <!--
            Labels (Y) scale
          -->
          <div class="dropdown">
            <CDropdown
              ref="scaleDropDown"
              :toggler-text="selectedScale"
              color="dark"
              no-caret
              nav
              placement="start"
            >
              <CDropdownItem
                v-for="d in fields"
                :key="d"
                @click="selectScale(d)"
              >
                {{ d }}
              </CDropdownItem>
            </CDropdown>
            <div class="dropdownLabel">
              Scale(Y-axis)
            </div>
          </div>
          <!--
            Datasets/Dimensions(D)
          -->
          <div class="dropdown">
            <CDropdown
              ref="dimensionsDropDown"
              :toggler-text="selectedDimension"
              color="dark"
              no-caret
              nav
              placement="start"
            >
              <CDropdownItem
                v-for="d in fields"
                :key="d"
                @click="selectDimension(d)"
              >
                {{ d }}
              </CDropdownItem>
            </CDropdown>
            <div class="dropdownLabel">
              Dimensions
            </div>
          </div>
          <!--
            Render Chart Button
          -->
          <CButton
            color="info"
            class="run active"
            @click="loadDataSet"
          >
            <span>
              <i
                class="far
                fa-play-circle"
                aria-hidden="true"
              />
              Reload Data
            </span>
          </CButton>
          <!--
            Render Chart Button
          -->
          <CButton
            color="info"
            class="run active"
            @click="renderChart"
          >
            <span>
              <i
                class="far
                fa-play-circle"
                aria-hidden="true"
              />
              Chart Data
            </span>
          </CButton>
        </flex-row>
      </div>
      <div
        id="linechart"
        hidden
      >
        <line-chart
          :chart-data="datacollection"
        />
      </div>
      <div
        id="barchart"
        hidden
      >
        <bar-chart
          :chart-data="datacollection"
        />
      </div>
      <div
        id="piechart"
        hidden
      >
        <pie-chart
          :chart-data="datacollection"
        />
      </div>
      <!-- <button @click="fillData()">Randomize</button> -->
      <!--
        Render Chart Button
      -->
      <CButton
        color="success"
        class="tablebutton active"
        @click="saveChart"
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
    </flex-col>
  </div>
</template>
<script>

import LineChart from './charts/LineChart.js'
import BarChart from './charts/BarChart.js'
import PieChart from './charts/PieChart.js'

export default {
  name: 'EditChart',
  components: {
    LineChart,
    BarChart,
    PieChart
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
      selectedDatabase: 'select',
      selectedDataSet: 'select',
      selectedChart: 'select',
      selectedLabel: 'select',
      selectedScale: 'select',
      selectedDimension: 'select',
      datacollection: null,
      dimensions: [],
      query: '',
      tables: [],
      charts: ['linechart', 'barchart', 'piechart'],
      dataSet: {}
    }
  },
  created () {
  },
  mounted () {
  },
  methods: {
    fillData () {
      this.datacollection = {
        labels: [''],
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
      this.selectedDataSet = 'select'
      this.selectedChart = 'select'
      this.selectedLabel = 'select'
      this.selectedScale = 'select'
      this.selectedDimension = 'select'
      for (var i in tables) {
        if (tables[i].type === 'dataset') {
          this.tables.push(tables[i])
        }
      }
      this.$refs.databaseDropDown.hide()
    },
    selectDataSet (ds) {
      this.selectedChart = 'select'
      this.selectedLabel = 'select'
      this.selectedScale = 'select'
      this.selectedDimension = 'select'
      var dataset = JSON.parse(ds.data)
      console.log(dataset)
      this.query = dataset.query
      this.fields = dataset.fields
      this.selectedDataSet = ds.tablename
      this.$refs.DataSetDropDown.hide()
      this.loadDataSet()
    },
    selectChart (chart) {
      this.$refs.chartDropDown.hide()
      console.log('this.selectedChart')
      console.log(this.selectedChart)
      if (this.selectedChart !== 'select') {
        document.getElementById(this.selectedChart).hidden = true
      }
      document.getElementById(chart).hidden = false
      this.selectedChart = chart
    },
    selectLabel (d) {
      this.selectedLabel = d
      this.$refs.labelDropDown.hide()
    },
    selectScale (d) {
      this.selectedScale = d
      this.$refs.scaleDropDown.hide()
    },
    selectDimension (d) {
      this.selectedDimension = d
      this.$refs.dimensionsDropDown.hide()
    },
    loadDataSet () {
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
              vm.dataSet = response.data.message
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
    },
    renderChart () {
      var vm = this
      console.log('this.dataSet')
      console.log(vm.dataSet)
      var labels = []
      var datasetsObj = {}
      /*
        We expect a stream of data like
        {key1:value1,key1:value2,key2:value1,ke2:value2}
        we need to sort these into objects such that
        key1:{data:[value1,value2]}}
        key2:{data:[value1,value2]}}
        key3:{data:[value1,value2]}}
        etc.
        first create a bunch of empty objects for each dataset
      */
      console.log('this.selectedDimension ' + vm.selectedDimension)
      console.log('this.selectedLabel ' + vm.selectedLabel)
      console.log('this.selectedScale ' + vm.selectedScale)
      for (var j in vm.dataSet) {
        if (vm.dataSet[j][vm.selectedLabel]) {
          if (!(labels.includes(vm.dataSet[j][vm.selectedLabel]))) {
            labels.push(vm.dataSet[j][vm.selectedLabel])
          }
        }
        if (vm.dataSet[j][vm.selectedDimension]) {
          if (datasetsObj[vm.dataSet[j][vm.selectedDimension]]) {
            if (vm.dataSet[j][vm.selectedScale]) {
              datasetsObj[vm.dataSet[j][vm.selectedDimension]].data.push(vm.dataSet[j][vm.selectedScale])
            }
          } else {
            datasetsObj[vm.dataSet[j][vm.selectedDimension]] = {}
            datasetsObj[vm.dataSet[j][vm.selectedDimension]].data = []
            datasetsObj[vm.dataSet[j][vm.selectedDimension]].fill = false
            datasetsObj[vm.dataSet[j][vm.selectedDimension]].background_color = '#afafaf'
            datasetsObj[vm.dataSet[j][vm.selectedDimension]].label = vm.dataSet[j][vm.selectedDimension]
            if (vm.dataSet[j][vm.selectedScale]) {
              datasetsObj[vm.dataSet[j][vm.selectedDimension]].data.push(vm.dataSet[j][vm.selectedScale])
            }
          }
        }
      }
      console.log('labels')
      console.log(labels)
      console.log('datasetsObj')
      console.log(datasetsObj)
      console.log('datasetsObj')
      console.log(datasetsObj)
      console.log(Object.values(datasetsObj))
      vm.datacollection = {
        labels: labels,
        datasets: Object.values(datasetsObj)
      }
      console.log('this.datacollection')
      console.log(vm.datacollection)
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
.run {
  margin: 10px;
  height: 35px;
}
.table-responsive {
  overflow-x: scroll;
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
