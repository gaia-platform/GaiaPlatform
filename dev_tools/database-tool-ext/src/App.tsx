import * as React from 'react';
import './App.css';

import gaia_logo from './gaia_logo.png';
import MainContainer from './containers/MainContainer'

function App() {
    return (
        <div className="App">
            <header className="App-header">
                <img src={gaia_logo} className="App-logo" alt="logo" />
                <h1 className="App-title">Gaia Platform Database Viewer</h1>
            </header>
            <MainContainer></MainContainer>
        </div>
    );
}

export default App;
