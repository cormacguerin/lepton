define(["exports", "./start-lit-element.js"], function (_exports, _startLitElement) {
  "use strict";

  Object.defineProperty(_exports, "__esModule", {
    value: true
  });
  _exports.SearchElement = _exports.$searchElement = void 0;

  function _templateObject_488eb6c0dfc311e9a9caf342f5dd962f() {
    var data = babelHelpers.taggedTemplateLiteral(["\n      <style>\n        :host { display: block; }\n        :host([hidden]) { display: none; }\n      </style>\n    "]);

    _templateObject_488eb6c0dfc311e9a9caf342f5dd962f = function _templateObject_488eb6c0dfc311e9a9caf342f5dd962f() {
      return data;
    };

    return data;
  }

  /**
   * @license
   * Copyright (c) 2018 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
   */
  // Import LitElement base class and html helper function
  var SearchElement =
  /*#__PURE__*/
  function (_LitElement) {
    babelHelpers.inherits(SearchElement, _LitElement);

    function SearchElement() {
      babelHelpers.classCallCheck(this, SearchElement);
      return babelHelpers.possibleConstructorReturn(this, babelHelpers.getPrototypeOf(SearchElement).apply(this, arguments));
    }

    babelHelpers.createClass(SearchElement, [{
      key: "render",

      /**
       * Define a template for the new element by implementing LitElement's
       * `render` function. `render` must return a lit-html TemplateResult.
       */
      value: function render() {
        return (0, _startLitElement.html)(_templateObject_488eb6c0dfc311e9a9caf342f5dd962f());
      }
    }]);
    return SearchElement;
  }(_startLitElement.LitElement); // Register the element with the browser


  _exports.SearchElement = SearchElement;
  customElements.define('search-element', SearchElement);
  var searchElement = {
    SearchElement: SearchElement
  };
  _exports.$searchElement = searchElement;
});