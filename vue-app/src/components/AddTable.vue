<template>
  <div class="input">
    <flex-row>
      <input
        v-model="value"
        placeholder="table name"
      >
      <CDropdown
        toggler-text="Data Type"
        nav
        placement="bottom-end"
      >
        <CDropdownItem
          v-for="dt in dataTypes"
          :key="dt"
          @click="selectDataType(dt)"
        >
          {{ dt }}
        </CDropdownItem>
      </CDropdown>
    </flex-row>
    <CButton
      class="btn active"
      color="info"
      @click="save"
    >
      Save
    </CButton>
  </div>
</template>
<script>

export default {
  name: 'AddTable',
  components: {
  },
  props: {
    database: {
      type: String,
      default: ''
    },
    value: {
      type: String,
      default: ''
    }
  },
  data () {
    return {
      dataTypes: [
        'serial', 'bigserial', 'int', 'bigint', 'real', 'date', 'varchar_64', 'varchar_2048', 'text'
      ],
      addDataModal: false
    }
  },
  created () {
  },
  methods: {
    showModal () {
      this.$refs.myModalRef.show()
    },
    hideModal () {
      this.$refs.myModalRef.hide()
    },
    selectDataType (dt) {
      console.log(dt)
    },
    save () {
      var vm = this
      this.$axios.get('https://35.239.29.200/api/addDatabase', {
        params: {
          database: vm.value
        }
      })
        .then(function (response) {
          if (response.data) {
            console.log(response.data)
            if (response.data.status === 'success') {
              vm.$parent.$parent.getDatabases()
              vm.$parent.$parent.addDataModal = false
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
.btn {
  margin: 10px;
}
input {
  margin: 10px;
  min-width: 100px;
  border: none;
  border-bottom: 2px solid #cfcfcf;
  outline: 0;
}
</style>
