<template>
  <div>
    <flex-col
      no-wrap
      class="container"
    >
      <div class="margin">
        <input
          v-model="chartName"
          class="left"
          placeholder="chart name"
        >
      </div>
      <flex-row>
        <div class="data">
          <fieldset class="fieldtitle">
            <legend class="legend">
              data
            </legend>
            <flex-row>
              <div class="dropdown">
                <CDropdown
                  ref="databaseDropDown"
                  :toggler-text="selectedDatabase"
                  color="dark"
                >
                  <CDropdownItem
                    v-for="d in dbs"
                    :key="d.key"
                    @click.native="selectDatabase(d.key,d.tables)"
                  >
                    {{ formatDatabaseName(d.key) }}
                  </cDropdownItem>
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
                >
                  <CDropdownItem
                    v-for="t in tables"
                    :key="t.tablename"
                    @click.native="selectDataSet(t)"
                  >
                    {{ t.tablename }}
                  </CDropdownItem>
                </CDropdown>
                <div class="dropdownLabel">
                  Data Set
                </div>
              </div>
            </flex-row>
          </fieldset>
          <fieldset class="fieldtitle">
            <legend class="legend">
              chart
            </legend>
            <flex-row>
              <!--
                Chart Selector
              -->
              <div class="dropdown">
                <CDropdown
                  ref="chartDropDown"
                  :toggler-text="selectedChart"
                  color="dark"
                >
                  <CDropdownItem
                    v-for="c in charts"
                    :key="c"
                    @click.native="selectChart(c)"
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
                >
                  <CDropdownItem
                    v-for="d in fields"
                    :key="d"
                    @click.native="selectLabel(d)"
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
                >
                  <CDropdownItem
                    v-for="d in fields"
                    :key="d"
                    @click.native="selectScale(d)"
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
                >
                  <CDropdownItem
                    v-for="d in fields"
                    :key="d"
                    @click.native="selectDimension(d)"
                  >
                    {{ d }}
                  </CDropdownItem>
                </CDropdown>
                <div class="dropdownLabel">
                  Dimensions
                </div>
              </div>
            </flex-row>
          </fieldset>
          <fieldset class="fieldtitle">
            <legend class="legend">
              colors
            </legend>
            <flex-row>
              <div class="dropdown">
                <CDropdown
                  ref="selectedColorFieldDropDown"
                  :toggler-text="selectedColorField"
                  color="dark"
                >
                  <CDropdownItem
                    v-for="d in dimensions"
                    :key="d"
                    @click.native="selectColorField(d)"
                  >
                    {{ d }}
                  </CDropdownItem>
                </CDropdown>
              </div>
              <div class="dropdown">
                <ColorPicker
                  ref="picker"
                  show-fallback
                  fallback-input-type="color"
                />
              </div>
            </flex-row>
          </fieldset>
        </div>
        <!-- Actual Chart -->
        <div>
          <div
            v-if="selectedChart === 'linechart'"
          >
            <line-chart
              :chart-data="datacollection"
              :options="dataoptions"
            />
          </div>
          <div
            v-else-if="selectedChart === 'barchart'"
          >
            <bar-chart
              :chart-data="datacollection"
              :options="dataoptions"
            />
          </div>
          <div
            v-else-if="selectedChart === 'piechart'"
          >
            <pie-chart
              :chart-data="datacollection"
              :options="dataoptions"
            />
          </div>
          <div
            v-else
          />
        </div>
      </flex-row>
      <flex-row>
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
            Render Chart
          </span>
        </CButton>
        <CButton
          class="run active"
          color="info"
          @click="save"
        >
          Save
        </CButton>
      </flex-row>
    </flex-col>
  </div>
</template>
<script>

import LineChart from './charts/LineChart.js'
import BarChart from './charts/BarChart.js'
import PieChart from './charts/PieChart.js'
import ColorPicker from './ColorPicker.vue'

export default {
  name: 'EditChart',
  components: {
    LineChart,
    BarChart,
    PieChart,
    ColorPicker
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
      selectedDimension: 'none',
      selectedColorField: 'select',
      selectedColor: '#efefef',
      datacollection: null,
      dataoptions: null,
      dimensions: [],
      fields: [],
      query: '',
      tables: [],
      charts: ['linechart', 'barchart', 'piechart'],
      dataSet: {},
      datasetOptions: {}
    }
  },
  created () {
  },
  mounted () {
    this.$watch(
      '$refs.picker.color', (n, o) => {
        if (this.datasetOptions[this.selectedColorField]) {
          this.datasetOptions[this.selectedColorField].strokeColor = n
          this.datasetOptions[this.selectedColorField].backgroundColor = n
          this.renderChart()
        }
      })
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
      this.selectedDimension = 'none'
      for (var i in tables) {
        if (tables[i].type === 'dataset') {
          this.tables.push(tables[i])
        }
      }
    },
    selectDataSet (ds) {
      var dataset = JSON.parse(ds.data)
      this.selectedDataSet = ds.tablename
      this.selectedChart = 'select'
      this.selectedLabel = 'select'
      this.selectedScale = 'select'
      this.selectedDimension = 'none'
      this.query = dataset.query
      this.fields = dataset.fields
      this.fields.unshift('none')
      this.$refs.DataSetDropDown.hide()
      this.loadDataSet()
    },
    selectChart (chart) {
      this.$refs.chartDropDown.hide()
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
      this.datasetOptions = {}
      this.dimensions = []
      for (const i in this.dataSet) {
        if (this.dataSet[i][this.selectedDimension]) {
          if (!(this.dimensions.includes(this.dataSet[i][this.selectedDimension]))) {
            this.dimensions.push(this.dataSet[i][this.selectedDimension])
          }
          this.datasetOptions[this.dataSet[i][this.selectedDimension]] = {}
        }
      }
      console.log(this.dimensions)
      this.$refs.dimensionsDropDown.hide()
    },
    selectColorField (d) {
      this.selectedColorField = d
      this.$refs.selectedColorFieldDropDown.hide()
    },
    loadDataSet () {
      var vm = this
      vm.$parent.$parent.getDatabases(function () {
        vm.$axios.get(vm.$SERVER_URI + '/api/runQuery', {
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
            }
          })
      })
    },
    renderChart () {
      var vm = this
      var labels = []
      var datasetsObj = {}
      /*
        We expect a stream of data like
        {key1:value1,key1:value2,key2:value1,ke2:value2}
        we need to sort these into objects such that
        etc.
        first create a bunch of empty objects for each dataset
      */
      console.log('vm.dataSet')
      console.log(vm.dataSet)
      for (var j in vm.dataSet) {
        // set labels
        if (vm.dataSet[j][vm.selectedLabel]) {
          if (!(labels.includes(vm.dataSet[j][vm.selectedLabel]))) {
            labels.push(vm.dataSet[j][vm.selectedLabel])
          }
        } else {
          console.log('no label skip')
          continue
        }
        // set data
        if (vm.selectedDimension === 'none') {
          console.log('no dimensions')
          if (datasetsObj[0]) {
            if (vm.dataSet[j][vm.selectedScale]) {
              datasetsObj[0].data.push(vm.dataSet[j][vm.selectedScale])
            } else {
              datasetsObj[0].data.push(null)
            }
            if (vm.dataSet[j][vm.selectedLabel]) {
              datasetsObj[0].labels.push(vm.dataSet[j][vm.selectedLabel])
            } else {
              datasetsObj[0].labels.push(null)
            }
          } else {
            datasetsObj[0] = {}
            datasetsObj[0].data = []
            datasetsObj[0].labels = []
            datasetsObj[0].fill = false
            datasetsObj[0].background_color = '#afafaf'
            datasetsObj[0].label = vm.SelectedLabel
            if (vm.dataSet[j][vm.selectedScale]) {
              datasetsObj[0].data.push(vm.dataSet[j][vm.selectedScale])
            } else {
              datasetsObj[0].data.push(null)
            }
            if (vm.dataSet[j][vm.selectedLabel]) {
              datasetsObj[0].labels.push(vm.dataSet[j][vm.selectedLabel])
            } else {
              datasetsObj[0].labels.push(null)
            }
          }
        } else if (vm.dataSet[j][vm.selectedDimension]) {
          if (datasetsObj[vm.dataSet[j][vm.selectedDimension]]) {
            if (vm.dataSet[j][vm.selectedScale]) {
              datasetsObj[vm.dataSet[j][vm.selectedDimension]].data.push(vm.dataSet[j][vm.selectedScale])
            } else {
              datasetsObj[vm.dataSet[j][vm.selectedDimension]].data.push(null)
            }
            if (vm.dataSet[j][vm.selectedLabel]) {
              datasetsObj[vm.dataSet[j][vm.selectedDimension]].labels.push(vm.dataSet[j][vm.selectedLabel])
            } else {
              datasetsObj[vm.dataSet[j][vm.selectedDimension]].labels.push(null)
            }
          } else {
            datasetsObj[vm.dataSet[j][vm.selectedDimension]] = {}
            datasetsObj[vm.dataSet[j][vm.selectedDimension]].data = []
            datasetsObj[vm.dataSet[j][vm.selectedDimension]].labels = []
            datasetsObj[vm.dataSet[j][vm.selectedDimension]].fill = false
            datasetsObj[vm.dataSet[j][vm.selectedDimension]].label = vm.dataSet[j][vm.selectedDimension]
            // set user options
            if (vm.datasetOptions[vm.dataSet[j][vm.selectedDimension]]) {
              for (const k in vm.datasetOptions[vm.dataSet[j][vm.selectedDimension]]) {
                console.log('k ' + k)
                console.log(vm.datasetOptions[vm.dataSet[j][vm.selectedDimension]][k])
                datasetsObj[vm.dataSet[j][vm.selectedDimension]][k] = vm.datasetOptions[vm.dataSet[j][vm.selectedDimension]][k]
              }
            }
            if (vm.dataSet[j][vm.selectedScale]) {
              datasetsObj[vm.dataSet[j][vm.selectedDimension]].data.push(vm.dataSet[j][vm.selectedScale])
            } else {
              datasetsObj[vm.dataSet[j][vm.selectedDimension]].data.push(null)
            }
            if (vm.dataSet[j][vm.selectedLabel]) {
              datasetsObj[vm.dataSet[j][vm.selectedDimension]].labels.push(vm.dataSet[j][vm.selectedLabel])
            } else {
              datasetsObj[vm.dataSet[j][vm.selectedDimension]].labels.push(null)
            }
          }
        }
      }
      console.log(Object.values(datasetsObj))
      vm.datacollection = {
        labels: labels,
        datasets: Object.values(datasetsObj)
      }
      console.log('vm.datacollection')
      console.log(vm.datacollection)
      vm.dataoptions = {
        responsive: true,
        legend: {
          display: true,
          position: 'bottom'
        },
        tooltips: {
          /*
          callbacks: {
            label: function (tooltipItem, data) {
              // console.log(data)
              // console.log(tooltipItem)
              var dataset = data.datasets[tooltipItem.datasetIndex]
              var index = tooltipItem.index
              return dataset.labels[index] + ': ' + dataset.data[index]
            }
          }
          */
        },
        onClick: function (e) {
          var activePointLabel = this.getElementsAtEvent(e)[0]
          console.log(this.getElementsAtEvent(e))
          console.log(activePointLabel)
        }
      }
      console.log('vm.dataoptions')
      console.log(vm.dataoptions)
    },
    save () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/createTable', {
        params: {
          database: vm.database,
          dataset: vm.dataSet,
          chartname: vm.chartName,
          datatype: vm.dataType
        }
      })
        .then(function (response) {
          if (response.data) {
            console.log(response.data)
            if (response.data.status === 'success') {
              vm.$parent.$parent.$parent.getDatabases(vm.tableName)
              vm.$parent.$parent.getTableSchema(vm.tableName)
              vm.$parent.$parent.addTableModal = false
            } else {
              console.log(response.data.message)
            }
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
.margin {
  margin: 5px;
}
.fieldtitle {
  border: 1px solid #efefef;
  margin: 5px;
  border-radius: 4px;
}
.custom-menu {
  background-color: darkgrey;
  color: white;
  overflow: scroll;
  max-height: 150px;
}
.left {
  float: left;
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
.legend {
  text-align: left;
  width: auto;
  max-width: auto;
  margin: 5px;
  font-size: 0.8em;
  padding-left: 5px;
  padding-right: 5px;
}
.data {
  width: 350px;
  margin-right: 10px;
}
input {
  margin: 5px;
  text-indent: 10px;
  min-width: 100px;
  border: none;
  border-bottom: 2px solid #cfcfcf;
  outline: 0;
}
</style>
