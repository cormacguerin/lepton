<template>
  <div class="card">
    <div class="button">
      <CButton
        color="primary"
        @click="getTableSchema"
      >
        {{ table }}
      </CButton>
    </div>
    <CCollapse
      :show="collapse"
      class="mt-2"
    >
      <CDataTable
        :items="items"
        :fields="fields"
        column-filter
        table-filter
        items-per-page-select
        hover
        sorter
        pagination
      >
        <template #status="{item}">
          <td>
            <CBadge :color="getBadge(item.status)">
              {{ item.status }}
            </CBadge>
          </td>
        </template>
        <template #show_details="{item, index}">
          <td class="py-2">
            <CButton
              color="primary"
              variant="outline"
              square
              size="sm"
              @click="toggleDetails(index)"
            >
              {{ details.includes(index) ? 'Hide' : 'Show' }}
            </CButton>
          </td>
        </template>
        <template #details="{item, index}">
          <CCollapse :show="details.includes(index)">
            <CCardBody>
              {{ index + 1 }} - {{ item }}
            </CCardBody>
          </CCollapse>
        </template>
      </CDataTable>
    </CCollapse>
  </div>
</template>
<script>

export default {
  name: 'TableItem',
  components: {
  },
  props: {
    table: {
      type: String,
      default: function () {
        return { }
      }
    },
    database: {
      type: String,
      default: function () {
        return { }
      }
    }
  },
  data () {
    return {
      items: [
      ],
      fields: [
      ],
      details: [
      ],
      collapse: false
    }
  },
  created () {
    console.log(this.schema)
  },
  methods: {
    getTableSchema () {
      this.collapse = !this.collapse
      if (this.items.length === 0 && this.fields.length === 0) {
        var vm = this
        this.$axios.get('https://' + this_SERVER_IP + '/api/getTableSchema', {
          params: {
            database: vm.database,
            table: vm.table
          }
        })
          .then(function (response) {
            console.log(response)
            if (response.data) {
              console.log(response.data.d)
              vm.items = response.data.d
              vm.fields = Object.keys(vm.items[0])
            }
          })
          .catch(function (error) {
            console.log(error)
          })
      }
    }
  }
}
</script>

<style scoped>
.schema-icon {
    transform: rotate(180deg);
    width: 100%;
}
.card {
    margin: 10px;
    padding: 10px;
    border: none;
    text-align: left;
    background-color: #fff;
}
.button {
    width: fit-content;
}
.schema {
    width: 20%;
    margin-left: auto;
    margin-right: auto;
}
.left {
    min-width: 100px;
}
.info {
    width: 75%;
}
</style>
