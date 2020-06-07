<template>
  <div class="dashboard">
    <flex-col
      justify="center"
      class="cards"
    >
      <IndexCard
        v-for="item in indices"
        :key="item.database"
        :database="item.database"
        :table="item.table"
        :column="item.column.join(',')"
        :display-field="item.display_field"
        :indexing="item.indexing"
        :serving="item.serving"
        :total="item.total"
        :indexed="item.indexed"
        :refreshed="item.refreshed"
      />
    </flex-col>
  </div>
</template>
<script>

import IndexCard from './IndexCard.vue'

export default {
  name: 'Indexing',
  components: {
    IndexCard
  },
  data () {
    return {
      indices: [],
      addChartModal: false
    }
  },
  created () {
    this.getIndexTables()
  },
  methods: {
    getIndexTables () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/getIndexingInfo', {
        params: {
        }
      })
        .then(function (response) {
          if (response.data) {
            console.log(response.data)
            vm.indices = response.data
          }
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
    width: 80%;
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
.addarea {
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
.hidden {
}
.modal {
  width: 1000px;
  max-width: 1024px;
}
</style>
