<template>
  <div class="result-card">
    <flex-col
      justify="left"
    >
      <div class="result-card-title">
        {{ title }}
      </div>
      <div class="result-card-snippet">
        {{ snippet }}
      </div>
      <div class="result-card-url">
        <a class="result-link">
          {{ url }}
        </a>
      </div>
    </flex-col>
    <flex-row class="debug">
      <div class="debugitem">
        weight : {{ weight }}
      </div>
      <div class="debugitem">
        tf : {{ tf }}
      </div>
      <div class="debugitem">
        tdscore : {{ tdscore }}
      </div>
      <div class="debugitem">
        wscore : {{ wscore }}
      </div>
      <div class="debugitem">
        docscore : {{ docscore }}
      </div>
      <div class="debugitem">
        score : {{ score }}
      </div>
    </flex-row>
  </div>
</template>

<script>

export default {
  name: 'ResultCard',
  components: {
  },
  props: {
    url: {
      type: String,
      default: ''
    },
    title: {
      type: String,
      default: ''
    },
    snippet: {
      type: String,
      default: ''
    },
    weight: {
      type: String,
      default: ''
    },
    tf: {
      type: String,
      default: ''
    },
    tdscore: {
      type: String,
      default: ''
    },
    wscore: {
      type: String,
      default: ''
    },
    docscore: {
      type: String,
      default: ''
    },
    score: {
      type: String,
      default: ''
    }
  }
}
</script>

<style scoped>
.result-card-title {
    font-size: 1.2em;
    font-weight: bold;
    text-align: left;
    color: white;
}
.result-card {
    background-color: #171f24;
    border-radius: 5px;
    padding: 10px;
    margin: 20px;
    color: #efefef;
    text-align: left;
}
.result-card-url {
  margin-top: 5px;
}
a.result-link {
    text-decoration: underline;
    color: #efefef;
    font-weight: bold;
}
.debug {
    margin-top: 10px;
}
.debugitem {
    padding: 5px;
    font-weight: bold;
    background-color: white;
    color: grey;
}
</style>
