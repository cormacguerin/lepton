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
		this.onSubmit = this.onSubmit.bind(this);
	}

	onSubmit(e) {
		console.log(e);
		e.preventDefault();
		var query = this.state.query;
		console.log(query);

		$.get("http://127.0.0.1:3000/search",
			{
				"query": query
			//}, function(data,status) {
			}, (data) => {
				if (data) {
					const results = JSON.parse(data);
					console.log(results);
					this.setState({
						results: results.nodes[0].nodes[0].debug_urls
					});
				}
			});
	}

	handleChange(e) {
		console.log(e.target.value);
		const  tv = e.target.value;
		this.setState({
			"query": tv
		})
	}

	render(){
		return (
			<React.Fragment>
				<div className="searchBox">
					<form className="form-horizontal">
						<input type="text" name="query" value={this.props.query} className="searchBox" onChange={this.handleChange} />
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

//						<button type="button" onClick={this.onSubmit.bind(this)} className="button">Search</button>

export default SearchBox;
