import React from 'react';
import $ from 'jquery';
import './SearchResult.css';

class SearchResult extends React.Component {

	constructor(props){
		super(props);
		this.state = {
			"topics": []
		};
	}

	render(){
		const topics = this.props.topics.split(',');
		if (topics.length>0) {
			this.state.topics = topics.reverse();
		}
		return (
			<React.Fragment>
				<div className="result">
					<a href={this.props.url}>{this.props.url}</a>
					<div className="snippet">
						{this.props.snippet}>{this.props.snippet}
					</div>
				</div>
				<br></br>
				<div className="topics">
					Topics : &nbsp;
						{this.state.topics.map(topic => <div class="topic">{topic},</div>)} &nbsp;
				</div>
				<br></br>
				<div className="debug">
					<p><span>[weight : {this.props.weight}] [wscore : {this.props.wscore}] [docscore : {this.props.docscore}] [score : {this.props.score}]</span></p>
				</div>
			</React.Fragment>
		);
	}
}

export default SearchResult;
