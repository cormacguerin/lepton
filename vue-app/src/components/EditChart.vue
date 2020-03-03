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
            <flex-col>
              <div
                v-show="selectedDimension === 'none'"
              >
                <flex-row>
                  <div class="dropdown">
                    <CDropdown
                      ref="selectedLabelsColorFieldDropDown"
                      :toggler-text="selectedLabelsColorField"
                      color="dark"
                    >
                      <CDropdownItem
                        v-for="d in labels"
                        :key="d"
                        @click.native="selectLabelsColorField(d)"
                      >
                        {{ d }}
                      </CDropdownItem>
                    </CDropdown>
                    <div class="dropdownLabel">
                      Labels
                    </div>
                  </div>
                  <div class="dropdown">
                    <CDropdown
                      ref="selectedLabelsColorPropertyFieldDropDown"
                      :toggler-text="selectedLabelsColorPropertyField"
                      color="dark"
                    >
                      <CDropdownItem
                        v-for="d in colorProperties"
                        :key="d"
                        @click.native="selectLabelsColorPropertyField(d)"
                      >
                        {{ d }}
                      </CDropdownItem>
                    </CDropdown>
                    <div class="dropdownLabel">
                      Property
                    </div>
                  </div>
                  <ColorPicker
                    ref="lpicker"
                    show-fallback
                    fallback-input-type="color"
                  />
                </flex-row>
              </div>
              <div
                v-show="selectedDimension !== 'none'"
              >
                <flex-row>
                  <div
                    class="dropdown"
                  >
                    <CDropdown
                      ref="selectedDimensionsColorFieldDropDown"
                      :toggler-text="selectedDimensionsColorField"
                      color="dark"
                    >
                      <CDropdownItem
                        v-for="d in dimensions"
                        :key="d"
                        @click.native="selectDimensionsColorField(d)"
                      >
                        {{ d }}
                      </CDropdownItem>
                    </CDropdown>
                    <div class="dropdownLabel">
                      Dimensions
                    </div>
                  </div>
                  <div class="dropdown">
                    <CDropdown
                      ref="selectedDimensionsColorPropertyFieldDropDown"
                      :toggler-text="selectedDimensionsColorPropertyField"
                      color="dark"
                    >
                      <CDropdownItem
                        v-for="d in colorProperties"
                        :key="d"
                        @click.native="selectDimensionsColorPropertyField(d)"
                      >
                        {{ d }}
                      </CDropdownItem>
                    </CDropdown>
                    <div class="dropdownLabel">
                      Property
                    </div>
                  </div>
                  <ColorPicker
                    ref="dpicker"
                    show-fallback
                    fallback-input-type="color"
                  />
                </flex-row>
              </div>
            </flex-col>
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
      selectedDimensionsColorField: 'select',
      selectedDimensionsColorPropertyField: 'select',
      selectedDimensionsColor: '#efefef',
      selectedLabelsColorPropertyField: 'select',
      selectedLabelsColorField: 'select',
      selectedLabelsColor: '#efefef',
      datacollection: null,
      dataoptions: null,
      dcolor: 'rgba(0, 0, 0, 0.1)',
      lcolor: 'rgba(0, 0, 0, 0.1)',
      dimensions: [],
      fields: [],
      labels: [],
      colorProperties: ['backgroundColor', 'borderColor', 'hoverBackgroundColor', 'hoverBorderColor', 'pointBackgroundColor', 'pointBorderColor', 'pointHoverBackgroundColor', 'pointHoverBorderColor'],
      query: '',
      tables: [],
      charts: ['linechart', 'barchart', 'piechart'],
      dataSet: {},
      datasetOptions: {}
    }
  },
  created () {
  },
  /*
   * A couple of watches to look at the color selections, these get added into a datasetOptions
   * object which read again in the render function and each property applied to the relevent data dimension
   */
  mounted () {
    this.$watch(
      '$refs.dpicker.color', (n, o) => {
        console.log('dpicker hit')
        /*
        var c = []
        for (var i = 0; i < this.labels.length; i++) {
          c.push(n)
        }
        */
        if (this.datasetOptions[this.selectedDimensionsColorField] === undefined) {
          this.datasetOptions[this.selectedDimensionsColorField] = {}
        }
        if (this.datasetOptions[this.selectedDimensionsColorField][this.selectedDimensionsColorPropertyField] === undefined) {
          //  this.datasetOptions[this.selectedDimensionsColorField][this.selectedDimensionsColorPropertyField] = []
        }
        this.datasetOptions[this.selectedDimensionsColorField][this.selectedDimensionsColorPropertyField] = n
        this.renderChart()
      })
    this.$watch(
      '$refs.lpicker.color', (n, o) => {
        console.log('lpicker hit')
        var x = 0
        for (var i in this.labels) {
          console.log(this.labels[i])
          console.log(x)
          if (this.selectedLabelsColorField === this.labels[i]) {
            break
          } else {
            x++
          }
        }
        if (this.selectedDimension === 'none') {
          if (this.datasetOptions[this.selectedLabel] === undefined) {
            this.datasetOptions[this.selectedLabel] = {}
          }
          if (this.datasetOptions[this.selectedLabel][this.selectedLabelsColorPropertyField] === undefined) {
            this.datasetOptions[this.selectedLabel][this.selectedLabelsColorPropertyField] = []
          }
          this.datasetOptions[this.selectedLabel][this.selectedLabelsColorPropertyField][x] = n
        } else {
          return
          /*
          for (var j in this.dimensions) {
            console.log('j')
            console.log(j)
            console.log('this.dimensions[j]')
            console.log(this.dimensions[j])
            this.datasetOptions[this.dimensions[j]][this.selectedLabelsColorPropertyField][x] = n
          }
          */
        }
        console.log('this.datasetOptions')
        console.log(this.datasetOptions)
        this.renderChart()
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
    selectDimensionsColorField (d) {
      this.selectedDimensionsColorField = d
      this.setDcolor()
      this.$refs.selectedDimensionsColorFieldDropDown.hide()
    },
    selectDimensionsColorPropertyField (d) {
      console.log('deb 1')
      this.selectedDimensionsColorPropertyField = d
      console.log('deb 2')
      this.setDcolor()
      console.log('deb 3')
      this.$refs.selectedDimensionsColorFieldDropDown.hide()
      console.log('deb 4')
    },
    selectLabelsColorField (d) {
      this.selectedLabelsColorField = d
      this.setLcolor()
      this.$refs.selectedLabelsColorFieldDropDown.hide()
    },
    selectLabelsColorPropertyField (d) {
      this.selectedLabelsColorPropertyField = d
      this.setLcolor()
      this.$refs.selectedLabelsColorPropertyFieldDropDown.hide()
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
    setDcolor () {
      if (this.datasetOptions[this.selectedDimensionsColorField] !== undefined) {
        if (this.datasetOptions[this.selectedDimensionsColorField][this.selectedDimensionsColorPropertyField]) {
          this.$refs.dpicker.color = this.datasetOptions[this.selectedDimensionsColorField][this.selectedDimensionsColorPropertyField]
        } else {
          this.$refs.dpicker.color = 'rgba(0, 0, 0, 0.1)'
        }
      }
    },
    setLcolor () {
      console.log('what lcolor?')
      if (this.datasetOptions[this.selectedLabelsColorField] !== undefined) {
        if (this.datasetOptions[this.selectedLabelsColorField][this.selectedLabelsColorPropertyField]) {
          this.$refs.lpicker.color = this.datasetOptions[this.selectedLabelsColorField][this.selectedLabelsColorPropertyField]
        } else {
          this.$refs.lpicker.color = 'rgba(0, 0, 0, 0.1)'
        }
      }
    },
    /*
     * Main logic here. We expect a stream of data like
     * {key1:value1,key1:value2,key2:value1,ke2:value2}
     * Loop over every dataset and access each element by user selected key
     * format & push the data into the chartjs data structured object (datasetsObj)
     */
    renderChart () {
      var vm = this
      var labels = []
      var datasetsObj = {}
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
        var obj
        if (vm.selectedDimension === 'none') {
          obj = vm.selectedLabel
        } else {
          obj = vm.dataSet[j][vm.selectedDimension]
        }
        if (datasetsObj[obj]) {
          if (obj) {
            datasetsObj[obj].data.push(vm.dataSet[j][vm.selectedScale])
          } else {
            datasetsObj[obj].data.push(null)
          }
          if (vm.dataSet[j][vm.selectedLabel]) {
            datasetsObj[obj].labels.push(vm.dataSet[j][vm.selectedLabel])
          } else {
            datasetsObj[obj].labels.push(null)
          }
        } else {
          var label
          if (vm.selectedDimension === 'none') {
            label = vm.selectedLabel
          } else {
            label = obj
          }
          datasetsObj[obj] = {}
          datasetsObj[obj].data = []
          datasetsObj[obj].labels = []
          datasetsObj[obj].fill = false
          datasetsObj[obj].label = label
          datasetsObj[obj].borderWidth = 1
          // set user options
          console.log('deb a')
          console.log(datasetsObj)
          if (vm.datasetOptions[obj]) {
            for (const k in vm.datasetOptions[obj]) {
              console.log('k ' + k)
              console.log(vm.datasetOptions[obj][k])
              datasetsObj[obj][k] = vm.datasetOptions[obj][k]
            }
          }
          // add data
          if (vm.dataSet[j][vm.selectedScale]) {
            datasetsObj[obj].data.push(vm.dataSet[j][vm.selectedScale])
          } else {
            datasetsObj[obj].data.push(null)
          }
          if (vm.dataSet[j][vm.selectedLabel]) {
            datasetsObj[obj].labels.push(vm.dataSet[j][vm.selectedLabel])
          } else {
            datasetsObj[obj].labels.push(null)
          }
        }
      }
      this.labels = labels
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
      var data = {}
      if (vm.datacollection) {
        data = vm.datacollection
      }
      var chart = {}
      chart.selectedChart = vm.selectedChart
      chart.selectedLabel = vm.selectedLabel
      chart.selectedScale = vm.selectedScale
      chart.selectedDimension = vm.selectedDimension
      chart.datasetOptions = vm.datasetOptions
      chart.dimensions = vm.dimensions
      this.$axios.get(this.$SERVER_URI + '/api/addChart', {
        params: {
          database: vm.selectedDatabase,
          dataset: vm.selectedDataSet,
          name: vm.chartName,
          chart: chart,
          data: data
        }
      })
        .then(function (response) {
          if (response.data) {
            console.log(response.data)
            if (response.data.status === 'success') {
              console.log('success')
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
