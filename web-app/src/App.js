import React from 'react';
import logo from './logo.svg';
import SearchBox from './SearchBox'
import './App.css';

function App() {
	return (
		<React.Fragment>
			<div className="App">
				<header className="header">
				</header>
				<div className="app-container">
					<SearchBox></SearchBox>
				</div>
			</div>
		</React.Fragment>
	);
}

export default App;
