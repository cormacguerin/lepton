<template>
  <div class="schema">
    <div
      class="tabsContainer"
    >
      <CNav
        variant="tabs"
        class="tabs"
        fade
      >
        <div
          @click="showTab('databaseTab')"
        >
          <CNavItem
            active
            class="databaseTab"
            title="Database"
          >
            <h2>Database</h2>
          </CNavItem>
        </div>
        <div
          @click="showTab('searchTab')"
        >
          <CNavItem
            class="searchTab"
            title="Search"
          >
            <h2>Search</h2>
          </CNavItem>
        </div>
      </CNav>
    </div>
    <div
      id="databaseTab"
    >
      <flex-row
        justify="center"
        class="cards"
      >
        <div class="flexgrow" />
        <div class="adddata">
          <CButton
            class="btn active"
            color="info"
            @click="addDataModal = true"
          >
            <span>
              Add Database
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
    <div
      id="searchTab"
      hidden
    >
      serch contents here
    </div>
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
      addDataModal: false,
      selectedTab: 'databaseTab'
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
    },
    showTab (t) {
      console.log(t)
      console.log(this.$refs[this.selectedTab])
      console.log(document.getElementById(this.selectedTab))
      document.getElementById(this.selectedTab).hidden = true
      document.getElementById(t).hidden = false
      // this.$refs[this.selectedTab].hidden = true
      // this.$refs[t].hidden = false
      this.selectedTab = t
    }
  }
}
</script>

<style scoped>
.schema {
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
