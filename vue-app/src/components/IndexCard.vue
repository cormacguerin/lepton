<template>
  <div class="card">
    <flex-col
      no-wrap
      class="container"
    >
      <div class="database">
        <flex-row
          class="left"
        >
          <div class="flexgrow indextitle">
            {{ database }} - {{ table }}
          </div>
          <div class="statusbarcontainer">
            <flex-col class="statusbar">
              <div
                :id="indexedid()"
                class="sindexed"
              />
              <div
                :id="staleid()"
                class="sstale"
              />
              <div
                :id="unindexedid()"
                class="sunindexed"
              />
            </flex-col>
          </div>
          <div class="flexgrow indexlegend">
            <flex-row>
              <div class="il" />
              <div>
                <i>indexed : {{ indexed }}</i>
              </div>
              <div class="sl" />
              <div>
                <i>stale : {{ refreshed }}</i>
              </div>
              <div class="ul" />
              <div>
                <i>unindexed : "{{ total - indexed }}"</i>
              </div>
            </flex-row>
          </div>
        </flex-row>
        <flex-row class="info">
          <div v-if="indexing">
            Indexing: Enabled
          </div>
          <div v-else>
            Indexing: Disabled
          </div>
          <div class="space">
           -
          </div>
          <div>
            Columns : {{ column }}
          </div>
        </flex-row>
      </div>
    </flex-col>
    <!--
    <CCollapse
      :show="collapse"
      :duration="300"
      class="mt-2 borderpad"
    >
        modal for editing a column (not visible until edit is clicked)
      <CModal
        title="Edit Table"
        color="info"
        :show.sync="editTableColumnModal"
      >
      </CModal>
        dataset edit area
    </CCollapse>
    -->
  </div>
</template>
<script>

export default {
  name: 'IndexCard',
  components: {
  },
  props: {
    database: {
      type: String,
      default: ''
    },
    table: {
      type: String,
      default: ''
    },
    column: {
      type: String,
      default: ''
    },
    displayfield: {
      type: String,
      default: ''
    },
    indexing: {
      type: Boolean,
      default: false
    },
    serving: {
      type: Boolean,
      default: false
    },
    total: {
      type: Number,
      default: 0
    },
    indexed: {
      type: Number,
      default: 0
    },
    refreshed: {
      type: Number,
      default: 0
    }
  },
  data () {
    return {
    }
  },
  watch: {
    total: function (val) {
      this.$watch(
        this.update()
      )
    }
  },
  mounted () {
    this.update()
  },
  methods: {
    indexedid () {
      return btoa(this.database + this.table + 'indexed')
    },
    staleid () {
      return btoa(this.database + this.table + 'staleid')
    },
    unindexedid () {
      return btoa(this.database + this.table + 'unindexed')
    },
    update () {
      if (this.total) {
        var _indexed = (this.indexed / this.total) * 100
        var _stale = (this.refreshed / this.total) * 100
        var _unindexed = ((this.total - this.indexed) / this.total) * 100
        document.getElementById(this.indexedid()).style.width = _indexed + '%'
        document.getElementById(this.indexedid()).style.width = _indexed + '%'
        document.getElementById(this.unindexedid()).style.width = _unindexed + '%'
        document.getElementById(this.staleid()).style.width = _stale + '%'
      } else {
        document.getElementById(this.unindexedid()).style.width = '100%'
      }
    }
  }
}
</script>

<style scoped>
.container {
  padding: 0px;
}
.absolute {
  position: absolute;
}
.card {
  min-height: 125px;
  margin-left: 10px;
  margin-right: 10px;
  margin-bottom: 10px;
  margin-top: 20px;
  border-radius: 3px;
  background-color: #fff;
  -webkit-box-shadow:0 2px 4px 0 #b2b5be;
     -moz-box-shadow:0 2px 4px 0 #b2b5be;
          box-shadow:0 2px 4px 0 #b2b5be;
}
.indextitle {
  width: 100%;
  border-radius-left-top: 3px;
  border-radius-right-top: 3px;
  text-align: left;
  background-color: #3285b9;
  padding: 5px;
  color: white;
  font-weight: bold;
}
.indexlegend {
  margin: 10px;
}
.statusbarcontainer {
  width: 100%;
  height: 20px;
}
.statusbar {
  position: absolute;
  padding: 10px;
  width: 100%;
  height: 30px;
}
.space {
  margin-left: 10px;
  margin-right: 10px;
}
.sindexed {
  background-color: #00bdff;
  height: 30px;
  width: 0%;
}
.sunindexed {
  background-color: #efefef;
  height: 30px;
  width: 0%;
}
.sstale {
  background-color: #91bfff;
  height: 15px;
  width: 0%;
}
.serror {
  background-color: #ff4136;
  height: 15px;
  width: 0%;
}
.il {
  background-color: #00bdff;
  width: 10px;
  height: 10px;
  margin-top: 5px;
  margin-right: 5px;
  margin-bottom: 5px;
}
.ul {
  background-color: #00bdff;
  width: 10px;
  height: 10px;
  margin: 5px;
}
.sl {
  background-color: #91bfff;
  width: 10px;
  height: 10px;
  margin: 5px;
}
.ul {
  background-color: #efefef;
  width: 10px;
  height: 10px;
  margin: 5px;
}
.el {
  background-color: red;
  width: 10px;
  height: 10px;
  margin: 5px;
}
.info {
  margin-left: 10px;
  margin-right: 10px;
}
</style>
