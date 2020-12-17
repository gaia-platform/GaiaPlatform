import React from "react";
import {
  Alert,
  Breadcrumb,
  BreadcrumbItem,
  Button,
  Container,
  Form,
  FormGroup,
  Label,
  Input,
  Col
} from "reactstrap";
import DatePicker from "react-datepicker";
import axios from "axios";

export default class CreateEvent extends React.Component {

  constructor(props) {
    super(props);
    this.state = {
      name: "",
      startTime: new Date(),
      endTime: new Date(),
      roomId: 0,
      staffId: 0,
      status: 0,
      data: {},
      error: "",
    };
    this.setName = this.setName.bind(this);
    this.setStartTime = this.setStartTime.bind(this);
    this.setEndTime = this.setEndTime.bind(this);
    this.setRoomId = this.setRoomId.bind(this);
    this.setStaffId = this.setStaffId.bind(this);
    this.handleSubmit = this.handleSubmit.bind(this);
  }

  setName(e) {
    this.setState({ name: e.target.value });
  }

  setStartTime(date) {
    this.setState({ startTime: date });
  }

  setEndTime(date) {
    this.setState({ endTime: date });
  }

  setRoomId(e) {
    this.setState({ roomId: Number(e.target.value) });
  }

  setStaffId(e) {
    this.setState({ staffId: Number(e.target.value) });
  }

  handleSubmit(e) {
    e.preventDefault();

    this.setState({ status: 0 });
    this.setState({ error: "" });
    this.setState({ warning: "" });

    axios
      .post("/event", {
        name: this.state.name,
        startTime: Math.floor(this.state.startTime.getTime() / 1000.0),
        endTime: Math.floor(this.state.endTime.getTime() / 1000.0),
        roomId: this.state.roomId,
        staffId: this.state.staffId,
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

    return (
      <>
        <div>
          <Breadcrumb>
            <BreadcrumbItem><a href="/">Home</a></BreadcrumbItem>
            <BreadcrumbItem>Events</BreadcrumbItem>
            <BreadcrumbItem active>Create</BreadcrumbItem>
          </Breadcrumb>
        </div>
        <Container>
          {status}
        </Container>

        <Container>
          <Form onSubmit={this.handleSubmit} >
            <FormGroup row={true}>
              <Col xs="4">
                <Label for="name">Name</Label>
                <Input
                  type="text"
                  name="name"
                  placeholder={this.state.name}
                  onChange={this.setName}
                />
              </Col>
            </FormGroup>

            <FormGroup row={true}>
              <Col>
                <Label for="startTime">Start Time</Label>
                <br />
                <DatePicker
                  showTimeSelect
                  timeFormat="HH:mm"
                  timeIntervals={15}
                  timeCaption="time"
                  dateFormat="MMMM d, yyyy h:mm aa"
                  selectStart
                  startDate={this.state.startTime}
                  selected={this.state.startTime}
                  onChange={date => this.setStartTime(date)}
                />
              </Col>
            </FormGroup>

            <FormGroup row={true}>
              <Col>
                <Label for="endTime">End Time</Label>
                <br />
                <DatePicker
                  showTimeSelect
                  timeFormat="HH:mm"
                  timeIntervals={15}
                  timeCaption="time"
                  dateFormat="MMMM d, yyyy h:mm aa"
                  selectEnd
                  endDate={this.state.endTime}
                  selected={this.state.endTime}
                  onChange={date => this.setEndTime(date)}
                />
              </Col>
            </FormGroup>

            <FormGroup row={true}>
              <Col xs="4">
                <Label for="roomId">Room Id</Label>
                <Input
                  type="number"
                  name="roomId"
                  placeholder={this.state.roomId}
                  onChange={this.setRoomId}
                />
              </Col>
            </FormGroup>

            <FormGroup row={true}>
              <Col xs="4">
                <Label for="staffId">Staff Id</Label>
                <Input
                  type="number"
                  name="staffId"
                  placeholder={this.state.staffId}
                  onChange={this.setStaffId}
                />
              </Col>
            </FormGroup>
            <Button>Submit</Button>
          </Form>
        </Container >
      </>
    );
  }
}
