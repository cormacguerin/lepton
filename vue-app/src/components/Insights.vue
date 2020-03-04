<template>
  <div class="searchbar">
    <flex-row>
      <div class="margin">
        <input
          v-model="query"
          placeholder=""
        >
      </div>
      <CButton
        class="active"
        color="info"
        @click="search"
      >
        Search
      </CButton>
    </flex-row>
  </div>
</template>
<script>

export default {
  name: 'Insights',
  data () {
    return {
      query: '',
      response: []
    }
  },
  methods: {
    search () {
      var vm = this
      this.$axios.get(this.$SERVER_URI + '/search', {
        params: {
          query: vm.query
        }
      })
        .then(function (response) {
          if (response.data) {
            console.log(response.data)
            this.response = response.data
          }
        })
    }
  }
}

</script>
<style scoped>
.searchbar {
    margin-top: 30px;
    margin-left: auto;
    margin-right: auto;
}
input {
    width: 600px;
    margin: 5px;
    margin-right: 20px;
    border: 1px #efefef solid;
    border-radius: 5px;
    background: white;
}
h2 {
    padding: 15px 0px 0px 0px;
    margin: 0px;
    text-align: center;
    font-size: 24px;
}
</style>
