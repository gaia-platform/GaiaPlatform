import React from "react";
import ReactDOM from "react-dom";
import "bootstrap/dist/css/bootstrap.min.css";
import "react-big-calendar/lib/css/react-big-calendar.css";
import "react-datepicker/dist/react-datepicker.css";
import App from "./App";
import reportWebVitals from "./reportWebVitals";

ReactDOM.render(
  // Use 'Fragment' instead of 'StrictMode' due to the following issue:
  // https://github.com/reactstrap/reactstrap/issues/1340
  <React.Fragment>
    <App />
  </React.Fragment>,
  document.getElementById("root")
);

// If you want to start measuring performance in your app, pass a function
// to log results (for example: reportWebVitals(console.log))
// or send to an analytics endpoint. Learn more: https://bit.ly/CRA-vitals
reportWebVitals();
