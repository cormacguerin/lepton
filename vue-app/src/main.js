import Vue from 'vue'
import CButton from '@coreui/vue'
import VueFlex from 'vue-flex'
import App from './App.vue'
import Axios from 'axios'
import VueRouter from 'vue-router'
import Home from './components/Home.vue'
import Data from './components/Data.vue'
import Visualize from './components/Visualize.vue'
import { cilPlus } from '@coreui/icons'
import 'vue-flex/dist/vue-flex.css'
import '@coreui/icons/css/all.css'
import '@coreui/icons/css/free.css'
import '@coreui/icons/css/brand.css'
import '@coreui/icons/css/all.min.css'
import './assets/coreui.min.css'

import '@fortawesome/fontawesome/styles.css'
import fontawesome from '@fortawesome/fontawesome'
import { library } from '@fortawesome/fontawesome-svg-core'
import { faPlus, faEllipsisV } from '@fortawesome/free-solid-svg-icons'
import { FontAwesomeIcon } from '@fortawesome/vue-fontawesome'

library.add(faPlus, faEllipsisV)

fontawesome.config = {
  autoAddCss: false
}

Vue.component('font-awesome-icon', FontAwesomeIcon)

Vue.config.productionTip = false

Vue.prototype.$axios = Axios

Vue.use(VueFlex)

Vue.use(VueRouter)

Vue.use(CButton)

Vue.use(FontAwesomeIcon)

Vue.config.productionTip = false

const routes = [
  { path: '/', name: 'home', component: Home },
  { path: '/data', name: 'data', component: Data },
  { path: '/visualize', name: 'visualize', component: Visualize }
]

const router = new VueRouter({
  mode: 'history',
  base: __dirname,
  routes
})

new Vue({
  render: h => h(App),
  icons: { cilPlus },
  router
}).$mount('#app')
