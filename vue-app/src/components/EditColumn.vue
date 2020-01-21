<template>
  <div>
    {{ columnName }}
    {{ dataType }}
    {{ characterMaximumLength }}
  </div>
</template>
<script>

export default {
  name: 'EditColumn',
  components: {
  },
  props: {
    column: {
      type: Object,
      default: function () {
        return {}
      }
    }
  },
  data () {
    return {
      column_name: '',
      data_type: '',
      character_maximum_length: ''
    }
  },
  computed: {
    columnName: function () {
      if (this.column) {
        return this.column.column_name
      } else {
        return ''
      }
    },
    dataType: function () {
      if (this.column) {
        return this.column.data_type
      } else {
        return ''
      }
    },
    characterMaximumLength: function () {
      if (this.column) {
        return this.column.character_maximum_length
      } else {
        return ''
      }
    }
  },
  created () {
  },
  methods: {
    getTableSchema (table) {
      console.log(table)
      if (this.collapse === true) {
        this.collapse = false
        if (this.selected === table) {
          this.selected = ''
          return
        }
      }
      var vm = this
      this.$axios.get('https://35.239.29.200/api/getTableSchema', {
        params: {
          database: vm.database,
          table: table
        }
      })
        .then(function (response) {
          if (response.data) {
            vm.items = response.data.d
            vm.fields = Object.keys(vm.items[0])
            vm.selected = table
            vm.collapse = true
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
h2 {
    width: 100%;
    padding: 15px 0px 0px 0px;
    margin: 0px;
    color: #777;
    text-align: center;
    font-size: 1.2em;
}
.database-icon {
    transform: rotate(180deg);
    width: 100%;
}
.cylinder {
    margin-top: -5px;
    width: 100%;
    height: 20px;
    border-top: 2px solid #4f8070;
    border-bottom: 1px solid #efefef;
    border-radius: 50%;
    background-color: #fff;
    -webkit-box-shadow:0 -4px 4px 0px rgba(89, 183, 187, 0.3), 0 0 0px rgba(0, 0, 0, 0.1) inset;
       -moz-box-shadow:0 -4px 4px 0px rgba(89, 183, 187, 0.3), 0 0 0px rgba(0, 0, 0, 0.1) inset;
            box-shadow:0 -4px 4px 0px rgba(89, 183, 187, 0.3), 0 0 0px rgba(0, 0, 0, 0.1) inset;
}
.buttons {
  margin-left: 10px;
  margin-right: 10px;
}
.tablebutton {
  margin: 10px;
}
.container {
    cursor: pointer;
}
.card {
    min-height: 125px;
    margin: 10px;
    padding: 10px;
    border: 1px solid #fff;
    border-radius: 3px;
    background-color: #fff;
    box-shadow: 0px 1px 5px rgba(0,0,0,0.3), 0 0px 0px rgba(0,0,0,0.22);
}
.database {
    width: 100px;
}
.left {
    min-width: 100px;
}
.info {
    width: 75%;
}
</style>
