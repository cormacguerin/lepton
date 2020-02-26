<template>
  <component
    :is="'div'"
    v-on-clickaway="hide"
    @click="checkClick($event)"
  >
    <div
      class="dropdown"
    >
      <slot name="toggler">
        <button
          class="btn"
          @click="toggle()"
        >
          {{ togglerText }}
        </button>
      </slot>
    </div>
    <div
      :class="menuClass"
      @click="hide()"
    >
      <slot />
    </div>
  </component>
</template>

<script>
import { mixin as clickaway } from 'vue-clickaway'

export default {
  name: 'CDropdown',
  components: {
  },
  mixins: [clickaway],
  props: {
    togglerText: {
      type: String,
      default: 'Dropdown'
    },
    visible: {
      type: Boolean,
      default: false
    }
  },
  data () {
    return {
      menuClass: 'dropdownmenu invisible'
    }
  },
  created () {
  },
  methods: {
    checkClick (e) {
      if (
        this.$scopedSlots.toggler &&
        this.$el.firstElementChild.contains(e.target)
      ) {
        this.toggle(e)
      }
    },
    open (t) {
      this.menuClass = 'dropdownmenu visible'
      this.visible = true
    },
    hide (t) {
      this.menuClass = 'dropdownmenu invisible'
      this.visible = false
    },
    toggle (t) {
      if (this.visible === false) {
        this.open()
      } else {
        this.hide()
      }
    }
  }
}
</script>

<style>
@media (max-width: 576px) {
  .desktopNav {
    display: none!important;
  }
}
@media (min-width: 576px) {
  .mobileNav {
    display: none!important;
  }
}
.invisible {
  visibility: hidden;
  opacity: 0;
  width: 0%;
  height: 0%;
  transition: visibility .3s .3s ease, opacity .3s 0s, width .3s ease, height .3s ease
}
.visible {
  visibility: visible;
  opacity: 1;
  transition: visibility .3s .3s ease, opacity .3s 0s, width .3s ease, height .3s ease
}
.dropdownmenu {
  position: absolute;
  overflow-y: scroll;
  overflow-x: hidden;
  will-change: transform;
  max-height: 300px;
  display: block;
  z-index: 10;
  min-width: 10rem;
  font-size: .875rem;
  text-align: left;
  list-style: none;
  background-clip: padding-box;
  border-radius: .25rem;
  color: #373847;
  background-color: #fff;
  border: 1px solid #bcbdc2;
  -webkit-box-shadow: 2px 2px 5px -2px rgba(0,0,0,0.75);
  -moz-box-shadow: 2px 2px 5px -2px rgba(0,0,0,0.75);
  box-shadow: 2px 2px 5px -2px rgba(0,0,0,0.75);
  scrollbar-3dlight-color:gold;
  scrollbar-arrow-color:blue;
  scrollbar-base-color:;
  scrollbar-darkshadow-color:blue;
  scrollbar-face-color:;
  scrollbar-highlight-color:;
  scrollbar-shadow-color:
}
.btn {
  min-width: 100px;
  border-radius: 4px;
  background-color: #151515;
  color: white;
}

</style>
