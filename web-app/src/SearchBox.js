import React from 'react';
import $ from 'jquery';
import SearchResult from './SearchResult.js';
import './SearchBox.css';

class SearchBox extends React.Component {

	constructor(props){
		super(props);
		this.state = {
			"query": '',
			"results": [],
			"hasResults": true 
		};
		this.handleChange = this.handleChange.bind(this);
		this.keyPress = this.keyPress.bind(this);
		this.onSubmit = this.onSubmit.bind(this);
		this.execute = this.execute.bind(this);
	}

	onSubmit(e) {
		e.preventDefault();
		var query = this.state.query;
		this.execute(query);
	}

	keyPress(e){
		if (e.keyCode == 13){
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
		console.log(q);
	//	$.get("https://35.239.29.200/search",
		$.get("http://127.0.0.1:3000/search",
			{
				"query": q
			}, (data) => {
				if (data) {
					const results = JSON.parse(data);
					console.log(results);
					console.log(results.query);
					if (results.items.length > 0) {
						this.setState({
							hasResults: true,
							results: results.items
						});
					} else {
						this.setState({
							hasResults: false,
							results: null
						});
					}
				}
			});
	}

	render() {
		return (
			<React.Fragment>
				<div className="searchBox">
					<form onSubmit={this.onSubmit.bind(this)} className="form-horizontal">
						<input type="text" name="query" value={this.props.query} className="searchBox" onChange={this.handleChange} onKeyDown={this.keyPress} />
					</form>
					<div className="searchButton" onClick={this.onSubmit.bind(this)}>
					</div>
				</div>
				{this.state.hasResults ?
					<div className="hasResults">
						{this.state.results.map(function(result) {
							return (<SearchResult 
									url={result.url}
									weight={result.weight}
									idf={result.idf}
									tf={result.tf}
									quality={result.quality}
									score={result.score}
								></SearchResult>)
						})}
					</div> : 
					<div className="noResults">
						No Results Found.
					</div>
				}
			</React.Fragment>
		);
	}
}

export default SearchBox;
