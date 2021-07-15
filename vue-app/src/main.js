import Vue from 'vue'
// import CoreuiVue from '@coreui/vue'
import { CButton, CSwitch, CModal, CCollapse, CDataTable, CNav, CNavItem } from '@coreui/vue'
import VueFlex from 'vue-flex'
import App from './App.vue'
import Axios from 'axios'
import VueRouter from 'vue-router'
import { cilPlus } from '@coreui/icons'
import { CDropdown, CDropdownItem } from './components/celements/celements.ts'
import Home from './components/Home.vue'
import Database from './components/Database.vue'
import Inference from './components/Inference.vue'
import Models from './components/Models.vue'
import Chart from './components/Chart.vue'
import Dashboard from './components/Dashboard.vue'
import Insights from './components/Insights.vue'
import Indexing from './components/Indexing.vue'
import Serving from './components/Serving.vue'
import ApiKeys from './components/ApiKeys.vue'
import Crawler from './components/Crawler.vue'
import 'vue-flex/dist/vue-flex.css'
import '@coreui/icons/css/all.css'
import '@coreui/icons/css/free.css'
import '@coreui/icons/css/brand.css'
import '@coreui/icons/css/all.min.css'
// import './assets/coreui.min.css'
// as mentioned on github styles need to be imported separately
// https://www.npmjs.com/package/@coreui/vue
import '@coreui/coreui/scss/coreui.scss'

// import '@fortawesome/fontawesome/styles.css'
import fontawesome from '@fortawesome/fontawesome'
import { library } from '@fortawesome/fontawesome-svg-core'
import { faPlus, faPlayCircle, faMinus, faBars, faEllipsisV } from '@fortawesome/free-solid-svg-icons'
import { FontAwesomeIcon } from '@fortawesome/vue-fontawesome'
import './assets/fonts/comfortaa.css'

library.add(faPlus, faPlayCircle, faMinus, faBars, faEllipsisV)

fontawesome.config = {
  autoAddCss: false
}

Vue.component('font-awesome-icon', FontAwesomeIcon)
Vue.component('CButton', CButton)
Vue.component('CSwitch', CSwitch)
Vue.component('CModal', CModal)
Vue.component('CDropdown', CDropdown)
Vue.component('CDropdownItem', CDropdownItem)
Vue.component('CCollapse', CCollapse)
Vue.component('CDataTable', CDataTable)
Vue.component('CNav', CNav)
Vue.component('CNavItem', CNavItem)

Vue.config.productionTip = false

Vue.prototype.$SERVER_URI = 'https://34.219.14.175'

Vue.prototype.$axios = Axios

Vue.use(VueFlex)
Vue.use(VueRouter)
// Vue.use(CoreuiVue)

Vue.use(FontAwesomeIcon)

Vue.config.productionTip = false

const routes = [
  { path: '/', name: 'home', component: Home },
  { path: '/database', name: 'database', component: Database },
  { path: '/models', name: 'models', component: Models },
  { path: '/inference', name: 'inference', component: Inference },
  { path: '/charts', name: 'charts', component: Chart },
  { path: '/dashboard', name: 'dashboard', component: Dashboard },
  { path: '/indexing', name: 'indexing', component: Indexing },
  { path: '/serving', name: 'serving', component: Serving },
  { path: '/insights', name: 'insights', component: Insights },
  { path: '/apikeys', name: 'apikeys', component: ApiKeys },
  { path: '/crawler', name: 'crawler', component: Crawler }
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
