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
import { html, LitElement } from 'lit-element';
//import { IronAjax } from 'iron-ajax';
import '@polymer/iron-ajax/iron-ajax.js';

export class StartLitElement extends LitElement {
	/**
	 * Define properties. Properties defined here will be automatically 
	 * observed.
	 */
	static get properties() {
		return {
			message: { type: String },
			searchquery: { type: String }
		};
	}

	/**  
	 * In the element constructor, assign default property values.
	 */
	constructor() {
		// Must call superconstructor first.
		super();

		// Initialize properties
		this.loadComplete = false;
		this.message = 'What would you like to search for today.';
	}

	/**
	 * Define a template for the new element by implementing LitElement's
	 * `render` function. `render` must return a lit-html TemplateResult.
	 */
	render() {
		return html`
			<style>
			:host { display: block; }
			:host([hidden]) { display: none; }
			</style>

			<iron-ajax
				id="ajaxsearch"
				method="GET"
				url="http://127.0.0.1:3000/search"
				handle-as="json"
				on-response="searchResponse"
				debounce-duration="300">
			</iron-ajax>

			<h1>Hi There!</h1>
			<p>${this.message}</p>

			<input id="query" name="searchQuery" type="input" .value=$(this.searchquery) @input=${this.handleInput} id="query"/>

			<button @click=${this.search}>Search</button>

			<search-element></search-element>
		`;
	}

	handleInput(e) {
		  this.searchquery = e.target.value;
	}

	search(e) {
		console.log("e.target.value");
		console.log(e.target.value);
		console.log("this.searchquery");
		console.log(this.searchquery);
		let ajaxsearch = this.shadowRoot.getElementById('ajaxsearch');
		ajaxsearch.params = {
			"query": this.searchquery
		}
		ajaxsearch.generateRequest();
	}

	searchResponse(data) {
		console.log(data.detail.response);
	}

	/**
	 * Implement firstUpdated to perform one-time work on first update:
	 * - Call a method to load the lazy element if necessary
	 * - Focus the checkbox
	 */
	firstUpdated() {
		this.loadLazy();

		const myInput = this.shadowRoot.getElementById('query');
		//myInput.focus();
	}

	/**
	 * If we need the lazy element && it hasn't already been loaded, 
	 * load it and remember that we loaded it.
	 */
	async loadLazy() {
		if(!this.loadComplete) {
			return import('./search-element.js').then((SearchElement) => {
				this.loadComplete = true;
				console.log("SearchElement loaded");
			}).catch((reason) => {
				this.loadComplete = false;
				console.log("SearchElement failed to load", reason);
			});
		}
	}
}

// Register the element with the browser
customElements.define('start-lit-element', StartLitElement);
