import Vue from 'vue'
import VueFlex from 'vue-flex'
// import "vue-flex/dist/vue-flex.css";
import App from './App.vue'

Vue.use(VueFlex)

Vue.config.productionTip = false

new Vue({
  render: h => h(App)
}).$mount('#app')