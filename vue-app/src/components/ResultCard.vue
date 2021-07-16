<template>
  <div
    class="result-card"
    @click="load"
  >
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
        <a href="{{ url }}" class="result-link">
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
  },
  methods: {
    load () {
      window.location.href = this.url
    }
  }
}
</script>

<style scoped>
.result-card-title {
    font-size: 1.2em;
    font-weight: bold;
    text-align: left;
    color: #171f24;
}
.result-card-title:hover {
    text-decoration: underline;
}
.result-card {
    background-color: #fafafa;
    color: #171f24;
    border-radius: 5px;
    padding: 10px;
    margin: 20px;
    color: #efefef;
    text-align: left;
    border: 1px solid #fafafa;
}
.result-card:hover {
    cursor: pointer;
    background-color: white;
    border: 1px solid #efefef;
    color: black;
}
.result-card-snippet {
    color: #171f24;
}
.result-card-url {
  margin-top: 5px;
}
a.result-link {
    text-decoration: underline;
    color: grey;
    font-weight: bold;
}
.debug {
    display: none;
    margin-top: 10px;
}
.debugitem {
    padding: 5px;
    font-weight: bold;
    background-color: white;
    color: grey;
}
</style>
