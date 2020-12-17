import React, { useState } from "react";
import { BrowserRouter as Router, Switch, Route } from "react-router-dom";
import {
  Collapse,
  Button,
  Jumbotron,
  Navbar,
  NavbarToggler,
  NavbarBrand,
  Nav,
  NavItem,
  NavLink,
  UncontrolledDropdown,
  DropdownToggle,
  DropdownMenu,
  DropdownItem,
  NavbarText,
} from "reactstrap";
import Register from "./Register";
import CreateEvent from "./CreateEvent";
import SchoolCalendar from "./Calendar";
import Students from "./Students";
import Parents from "./Parents";
import Staff from "./Staff";
import Event from "./Event";
import Buildings from "./Buildings";

export default function App() {
  const [isOpen, setIsOpen] = useState(false);

  const toggle = () => setIsOpen(!isOpen);

  return (
    <Router>
      <div>
        <div>
          <Navbar color="light" light expand="md">
            <NavbarBrand href="/">Gaia School</NavbarBrand>
            <NavbarToggler onClick={toggle} />
            <Collapse isOpen={isOpen} navbar>
              <Nav className="mr-auto" navbar>
                <UncontrolledDropdown nav inNavbar>
                  <DropdownToggle nav caret>
                    People
                </DropdownToggle>
                  <DropdownMenu>
                    <DropdownItem href="/students">Students</DropdownItem>
                    <DropdownItem href="/parents">Parents</DropdownItem>
                    <DropdownItem href="/staff">Staff</DropdownItem>
                    <DropdownItem divider />
                    <DropdownItem href="/register">Register</DropdownItem>
                  </DropdownMenu>
                </UncontrolledDropdown>
                <UncontrolledDropdown nav inNavbar>
                  <DropdownToggle nav caret>
                    Events
                </DropdownToggle>
                  <DropdownMenu>
                    <DropdownItem href="/calendar">Calendar</DropdownItem>
                    <DropdownItem divider />
                    <DropdownItem href="/create">Create</DropdownItem>
                  </DropdownMenu>
                </UncontrolledDropdown>
                <NavItem>
                  <NavLink href="/buildings">Buildings</NavLink>
                </NavItem>
              </Nav>
              <NavbarText>Login</NavbarText>
            </Collapse>
          </Navbar>
        </div>

        <div>
          <Switch>
            <Route path="/staff">
              <Staff />
            </Route>
            <Route path="/students">
              <Students />
            </Route>
            <Route path="/parents">
              <Parents />
            </Route>
            <Route path="/register">
              <Register />
            </Route>
            <Route path="/create">
              <CreateEvent />
            </Route>
            <Route path="/calendar">
              <SchoolCalendar />
            </Route>
            <Route path="/buildings">
              <Buildings />
            </Route>
            <Route path="/event/:eventId">
              <Event />
            </Route>
            <Route path="/">
              <Home />
            </Route>
          </Switch>
        </div>
      </div>
    </Router >
  );
}

function Home() {
  return (
    <div>
      <Jumbotron>
        <h1 className="display-3">Gaia School Demo</h1>
        <p className="lead">
          This is a web app to demo Gaia school management system.
        </p>
        <hr className="my-2" />
        <p>
          It uses Gaia as backend storage and business logic (rules) execution.
        </p>
        <p className="lead">
          <Button color="primary" href="https://www.gaiaplatform.io/">
            Learn More
          </Button>
        </p>
      </Jumbotron>
    </div>
  );
}

