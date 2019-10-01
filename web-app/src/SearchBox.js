import React from 'react';
import $ from 'jquery';
import SearchResult from './SearchResult.js';
import './SearchBox.css';

class SearchBox extends React.Component {

	constructor(props){
		super(props);
		this.state = { "query": '' };
		this.state = { "results": [] };
		this.handleChange = this.handleChange.bind(this);
		this.keyPress = this.keyPress.bind(this);
		this.onSubmit = this.onSubmit.bind(this);
		this.execute = this.execute.bind(this);
	}

	onSubmit(e) {
		console.log(e);
		e.preventDefault();
		var query = this.state.query;
		console.log(query);
		this.execute(query);
	}

	keyPress(e){
		console.log(e);
		if(e.keyCode == 13){
			this.execute(this.state.query);
		}
	}

	handleChange(e) {
		console.log(e.target.value);
		const  tv = e.target.value;
		this.setState({
			"query": tv
		})
	}

	execute(q) {
	//	$.get("https://35.239.29.200/search",
		$.get("https://127.0.0.1/search",
			{
				"query": q
			}, (data) => {
				if (data) {
					const results = JSON.parse(data);
					console.log(results);
					if (results.debug_urls) {
						this.setState({
							results: results.debug_urls
						});
					}
				}
			});
	}

	render(){
		return (
			<React.Fragment>
				<div className="searchBox">
					<form className="form-horizontal">
						<input type="text" name="query" value={this.props.query} className="searchBox" onChange={this.handleChange} onKeyDown={this.keyPress} />
					</form>
					<div className="searchButton" onClick={this.onSubmit.bind(this)}>
					</div>
				</div>
				<div>
					{this.state.results.map(function(result){
						return (<SearchResult result={result}></SearchResult>)
					})}
				</div>
			</React.Fragment>
		);
	}
}

export default SearchBox;
