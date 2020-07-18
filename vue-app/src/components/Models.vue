<template>
  <div class="database">
    <flex-row
      justify="center"
      class="cards"
    >
      <div class="flexgrow" />
      <div class="adddata">
        <CButton
          class="btn active"
          color="info"
          @click="addModelModal = true"
        >
          <span>
            <i
              class="fa fa-plus"
              aria-hidden="true"
            />
            Model
          </span>
        </CButton>
        <CModal
          title="Add Model"
          color="info"
          :show.sync="addModelModal"
        >
          <template #footer-wrapper>
            <div class="hidden" />
          </template>
          <EditModel />
        </CModal>
      </div>
    </flex-row>
    <div
      justify="center"
      class="cards"
    >
      <ModelCard
        v-for="(value, key) in dbs"
        :key="key"
        :models="value.key"
        :tables="value.tables"
      />
    </div>
  </div>
</template>
<script>

import ModelCard from './ModelCard.vue'
import EditModel from './EditModel.vue'

export default {
  name: 'Model',
  components: {
    ModelCard,
    EditModel
  },
  data () {
    return {
      dbs: {},
      addModelModal: false
    }
  },
  created () {
    this.getModels()
  },
  methods: {
    getModels () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/api/getModels', {
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
.database {
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
</style>
