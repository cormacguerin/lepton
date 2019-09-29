import React from 'react';
import $ from 'jquery';
import './SearchResult.css';

class SearchResult extends React.Component {

	constructor(props){
		super(props);
	}

	render(){
		return (
			<div className="result">
				<a href={this.props.result}>{this.props.result}</a>
			</div>
		);
	}
}

export default SearchResult;
