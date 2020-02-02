<template>
  <div class="data">
    <flex-row
      justify="center"
      class="cards"
    >
      <div class="headertitle">
        <h2>Data</h2>
      </div>
      <div class="flexgrow" />
      <div class="adddata">
        <CButton
          class="btn active"
          color="info"
          @click="addDataModal = true"
        >
          <span>
            Add
            <i
              class="fa fa-plus"
              aria-hidden="true"
            />
          </span>
        </CButton>
        <CModal
          title="Add Data"
          color="info"
          :show.sync="addDataModal"
        >
          <template #footer-wrapper>
            <div class="hidden" />
          </template>
          <EditDatabase />
        </CModal>
      </div>
    </flex-row>
    <flex-col
      justify="center"
      class="cards"
    >
      <DatabaseCard
        v-for="(value, key) in dbs"
        :key="key"
        :database="value.key"
        :tables="value.tables"
      />
    </flex-col>
  </div>
</template>
<script>

import DatabaseCard from './DatabaseCard.vue'
import EditDatabase from './EditDatabase.vue'

export default {
  name: 'Data',
  components: {
    DatabaseCard,
    EditDatabase
  },
  data () {
    return {
      dbs: {
      },
      addDataModal: false
    }
  },
  created () {
    this.getDatabases()
  },
  methods: {
    getDatabases () {
      var vm = this
      this.$axios.get('https://35.239.29.200/api/getDatabases', {
      })
        .then(function (response) {
          if (response.data) {
            vm.dbs = response.data
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
.data {
    width: 100%;
    height: 100vh;
    background: white;
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
.adddata {
    margin-top: 50px;
    margin-right: 10px;
}
h2 {
    padding: 15px 0px 0px 0px;
    margin: 0px;
    text-align: center;
    font-size: 24px;
}
.hidden {
}
</style>
