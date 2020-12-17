import React from "react";
import {
  Alert,
  Breadcrumb,
  BreadcrumbItem,
  Button,
  Container,
  Col,
  Form,
  FormGroup,
  Label,
  Input
} from "reactstrap";
import DatePicker from "react-datepicker";
import axios from "axios";

class Register extends React.Component {
  constructor(props) {
    super(props);
    this.state = {
      firstName: "Jane",
      lastName: "Doe",
      birthdate: new Date(),
      role: "student",
      hiredDate: new Date(),
      studentId: 0,
      relationship: "mother",
      status: 0,
      data: {},
      error: "",
    };
    this.setFirstName = this.setFirstName.bind(this);
    this.setLastName = this.setLastName.bind(this);
    this.setRole = this.setRole.bind(this);
    this.setBirthdate = this.setBirthdate.bind(this);
    this.setHiredDate = this.setHiredDate.bind(this);
    this.setStudentId = this.setStudentId.bind(this);
    this.setRelationship = this.setRelationship.bind(this);
    this.handleSubmit = this.handleSubmit.bind(this);
  }

  setFirstName(e) {
    this.setState({ firstName: e.target.value });
  }

  setLastName(e) {
    this.setState({ lastName: e.target.value });
  }

  setRole(e) {
    this.setState({ role: e.target.value });
  }

  setBirthdate(date) {
    this.setState({ birthdate: date });
  }

  setHiredDate(date) {
    this.setState({ hiredDate: date });
  }

  setStudentId(e) {
    this.setState({ studentId: Number(e.target.value) });
  }

  setRelationship(e) {
    this.setState({ relationship: e.target.value });
  }

  handleSubmit(e) {
    e.preventDefault();

    this.setState({ status: 0 });
    this.setState({ error: "" });
    this.setState({ warning: "" });

    axios
      .post("/person", {
        firstName: this.state.firstName,
        lastName: this.state.lastName,
        birthdate: Math.floor(this.state.birthdate.getTime() / 1000.0),
        role: this.state.role,
        hiredDate: Math.floor(this.state.hiredDate.getTime() / 1000.0),
        studentId: this.state.studentId,
        relationship: this.state.relationship,
      })
      .then((res) => {
        this.setState({ status: res.status });
        if (res.status === 200) {
          this.setState({ data: res.data });
        } else {
          this.setState({ warning: JSON.stringify(res) });
        }
      })
      .catch((error) => {
        if (error.response) {
          this.setState({ status: error.response.status });
          if (error.response.status === 405) {
            this.setState({ data: error.response.data });
          } else {
            this.setState({ error: JSON.stringify(error.response) });
          }
        } else if (error.request) {
          this.setState({ error: JSON.stringify(error.request) });
        } else {
          this.setState({ error: error.message });
        }
      });
  }

  render() {
    let status;
    let items = [];
    if (this.state.error) {
      status = (
        <Alert color="danger">
          <h4 className="alert-heading">Error</h4>
          <hr />
          <p>{this.state.error}</p>
        </Alert>
      );
    } else if (this.state.status === 200) {
      for (const key of Object.keys(this.state.data)) {
        items.push(
          <li key={key}>
            {key} : {this.state.data[key]}
          </li>
        );
      }
      status = (
        <Alert color="success">
          <h4 className="alert-heading">Success</h4>
          <p>A person of {this.state.role} role is registered.</p>
          <hr />
          <ul>{items}</ul>
        </Alert>
      );
    } else if (this.state.status === 405) {
      status = (
        <Alert color="warning">
          <h4 className="alert-heading">Warning</h4>
          <hr />
          <p>{this.state.data.what}</p>
        </Alert>
      );
    }

    let more;
    if (this.state.role === "staff") {
      more = (
        <FormGroup row={true}>

          <Col xs="4">
            <Label for="hiredDate">Hired Date</Label>
            <br />
            <DatePicker
              name="hiredDate"
              peekNextMonth
              showMonthDropdown
              showYearDropdown
              dropdownMode="select"
              selected={this.state.hiredDate}
              onChange={date => this.setHiredDate(date)}
            />
          </Col>
        </FormGroup>
      );
    } else if (this.state.role === "parent") {
      more = (
        <>
          <FormGroup row={true}>
            <Col xs="4">
              <Label for="studentId">Student Id</Label>
              <Input
                type="number"
                name="studentId"
                placeholder={this.state.studentId}
                onChange={this.setStudentId}
              />
            </Col>
          </FormGroup>
          <FormGroup row={true}>
            <Col xs="4">
              <Label for="relationship">Relationship (to the student)</Label>
              <Input
                type="select"
                name="relationship"
                onChange={this.setRelationship}
              >
                <option value="mother">Mother</option>
                <option value="father">Father</option>
              </Input>
            </Col>
          </FormGroup>
        </>
      );
    }

    return (
      <>
        <div>
          <Breadcrumb>
            <BreadcrumbItem><a href="/">Home</a></BreadcrumbItem>
            <BreadcrumbItem>People</BreadcrumbItem>
            <BreadcrumbItem active>Register</BreadcrumbItem>
          </Breadcrumb>
        </div>
        <Container>
          {status}
        </Container>

        <Container>
          <Form onSubmit={this.handleSubmit}>
            <FormGroup row={true}>
              <Col xs="4">
                <Label for="firstName">First Name</Label>
                <Input
                  type="text"
                  name="firstName"
                  placeholder={this.state.firstName}
                  onChange={this.setFirstName}
                />
              </Col>
            </FormGroup>

            <FormGroup row={true}>
              <Col xs="4">
                <Label for="lastName">Last Name</Label>
                <Input
                  type="text"
                  name="lastName"
                  placeholder={this.state.lastName}
                  onChange={this.setLastName}
                />
              </Col>
            </FormGroup>

            <FormGroup row={true}>
              <Col>
                <Label for="birthdate">Birthdate</Label>
                <br />
                <DatePicker
                  name="birthdate"
                  peekNextMonth
                  showMonthDropdown
                  showYearDropdown
                  dropdownMode="select"
                  selected={this.state.birthdate}
                  onChange={date => this.setBirthdate(date)}
                />
              </Col>
            </FormGroup>

            <FormGroup row={true}>
              <Col xs="4">
                <Label for="role">Select a role</Label>
                <Input type="select" name="role" onChange={this.setRole}>
                  <option value="student">Student</option>
                  <option value="staff">Staff</option>
                  <option value="parent">Parent</option>
                </Input>
              </Col>
            </FormGroup>

            {more}

            <Button>Submit</Button>
          </Form>
        </Container>
      </>
    );
  }
}

export default Register;
