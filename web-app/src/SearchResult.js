import React from 'react';
import $ from 'jquery';
import './SearchResult.css';

class SearchResult extends React.Component {

	constructor(props){
		super(props);
	}

	render(){
		return (
			<React.Fragment>
				<div className="result">
					<a href={this.props.url}>{this.props.url}</a>
				</div>
				<div className="debug">
					<p><span>[weight : {this.props.weight}] [tdscore : {this.props.tdscore}] [docscore : {this.props.docscore}] [score : {this.props.score}]</span></p>
				</div>
			</React.Fragment>
		);
	}
}

export default SearchResult;
