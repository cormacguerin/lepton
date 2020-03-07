<template>
  <div class="dashboard">
    <flex-row
      justify="center"
      class="cards"
    >
      <div class="flexgrow" />
      <div class="addchart">
        <CButton
          class="btn active"
          color="info"
          @click="addChartModal = true"
        >
          <span>
            <i
              class="fa fa-plus"
              aria-hidden="true"
            />
            Chart
          </span>
        </CButton>
        <CModal
          title="Add Chart"
          color="info"
          size="lg"
          :show.sync="addChartModal"
        >
          <template #footer-wrapper>
            <div class="hidden" />
          </template>
          <EditChart
            :dbs="dbs"
          />
        </CModal>
      </div>
    </flex-row>
    <flex-col
      justify="center"
      class="cards"
    >
      <ChartCard
        v-for="(value, key) in dbs"
        :key="key"
        :database="value.key"
        :tables="value.tables"
        :datasets="value.datasets"
      />
    </flex-col>
  </div>
</template>
<script>

import ChartCard from './ChartCard.vue'
import EditChart from './EditChart.vue'

export default {
  name: 'Schema',
  components: {
    ChartCard,
    EditChart
  },
  data () {
    return {
      dbs: {
      },
      addChartModal: false
    }
  },
  created () {
    this.getDatabases()
  },
  methods: {
    getDatabases (f) {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/getDatabases', {
      })
        .then(function (response) {
          if (response.data) {
            vm.dbs = response.data
          }
          if (f) {
            f()
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
.dashboard {
    width: 100%;
}
.cards {
    max-width: 1000px;
    margin-left: auto;
    margin-right: auto;
}
.flexgrow {
    flex-grow: 2;
}
.headertitle {
    margin-top: 50px;
    margin-left: 10px;
}
.addchart {
    margin-top: 50px;
    margin-right: 10px;
}
h2 {
    padding: 15px 0px 0px 0px;
    margin: 0px;
    text-align: center;
    font-size: 24px;
}
.databaseTab {
}
.searchTab {
}
.tabs {
    background-color: #171f24;
    width: 1000px;
    margin-left: auto;
    margin-right: auto;
}
.nav-tabs {
    border: 0px;
}
.tabsContainer {
    background-color: #171f24;
}
.hidden {
}
.modal {
  width: 1000px;
  max-width: 1024px;
}
</style>
