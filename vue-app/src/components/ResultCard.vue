<template>
  <div class="result-card">
    <flex-col
      justify="left"
    >
      <div class="result-card-snippet">
        {{ title }}
      </div>
      <div class="result-card-url">
        {{ snippet }}
      </div>
      <div class="result-card-url">
        <a class="result-link">
          {{ url }}
        </a>
      </div>
    </flex-col>
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
    }
  }
}
</script>

<style>
.result-card-title {
    font-size: 1.2em;
    font-weight: bold;
    text-align: left;
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
</style>
