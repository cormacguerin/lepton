<template>
  <div class="card">
    <div class="button">
      <CButton
        color="info"
        :class="m-1"
        @click="getTableSchema"
      >
        {{ schema }}
      </CButton>
    </div>
    <CCollapse
      :show="collapse"
      class="m-2"
    >
      <TableSchema
        v-for="s in schema"
        :key="s.schema"
        :schema="s.schema"
      />
    </CCollapse>
  </div>
</template>
<script>

import { CButton } from '@coreui/vue'

export default {
  name: 'TableSchema',
  components: {
    CButton
  },
  props: {
    table: {
      type: String,
      default: function () {
        return { }
      }
    }
  },
  data () {
    return {
      collapse: false
    }
  },
  created () {
    console.log(this.schema)
  },
  methods: {
    getTableSchema () {
      var vm = this
      this.$axios.get('https://35.239.29.200/api/getTableSchema', {
        params: {
          table: vm.table
        }
      })
        .then(function (response) {
          console.log(response)
          if (response.data) {
            console.log(response.data)
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
    font-size: 24px;
}
.schema-icon {
    transform: rotate(180deg);
    width: 100%;
}
.container {
    width: 100%;
    cursor: pointer;
}
.card {
    border: none;
    margin: 5px;
    padding: 5px;
    background-color: #fff;
    border: none;
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
