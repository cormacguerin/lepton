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
          </div>
          <!--
            Label and dimensions
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
          </div>
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
      selectedDatabase: 'database',
      selectedDataSet: 'dataset',
      selectedChart: 'chart',
      selectedDimension: 'dimension',
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
      for (var i in tables) {
        if (tables[i].type === 'dataset') {
          this.tables.push(tables[i])
        }
      }
      this.$refs.databaseDropDown.hide()
    },
    selectDataSet (ds) {
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
      if (this.selectedChart !== 'chart') {
        document.getElementById(this.selectedChart).hidden = true
      }
      document.getElementById(chart).hidden = false
      this.selectedChart = chart
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
      if (this.dataSet) {
        console.log('this.dataSet')
        console.log(this.dataSet)
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
          first create a bunch of empty objects for each key
        */
        for (var i in this.dataSet[0]) {
          var dataset = {}
          dataset.data = []
          dataset.fill = false
          dataset.backgroundColor = '#afafaf'
          console.log('i ' + i)
          // if (i !== this.selectedDimension) {
          datasetsObj[i] = dataset
          // }
        }
        console.log('datasetsObj begin')
        console.log(datasetsObj)
        console.log('this.selectedDimension ' + this.selectedDimension)
        for (var j in this.dataSet) {
          console.log('j ' + j)
          for (var k in this.dataSet[j]) {
            console.log('k ' + k)
            if (k === this.selectedDimension) {
              labels.push(this.dataSet[j][k])
            }
            console.log(this.dataSet[j][k])
            datasetsObj[k].label = k
            datasetsObj[k].data.push(this.dataSet[j][k])
          }
        }
        console.log('datasetsObj')
        console.log(datasetsObj)
        console.log(Object.values(datasetsObj))
        this.datacollection = {
          labels: labels,
          datasets: Object.values(datasetsObj)
        }
        console.log('this.datacollection')
        console.log(this.datacollection)
      } else {
        console.log('no chart dataset to render')
      }
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
