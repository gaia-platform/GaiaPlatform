import React from "react";
import { Breadcrumb, BreadcrumbItem, Container, ListGroupItem, ListGroup } from "reactstrap";
import { withRouter } from "react-router";
import axios from "axios";

class Event extends React.Component {
  state = {
    event: {},
    students: []
  }

  componentDidMount() {
    const eventId = this.props.match.params.eventId;
    axios.get(`/event/${eventId}`).then((res) => {
      if (res.status === 200 && res.data) {
        const event = res.data;
        this.setState({ event });
      }
    });
    axios.get(`/event/${eventId}/participant`).then((res) => {
      if (res.status === 200 && res.data) {
        const students = res.data;
        this.setState({ students });
      }
    });
  }

  render() {
    return (
      <>
        <div>
          <Breadcrumb>
            <BreadcrumbItem><a href="/">Home</a></BreadcrumbItem>
            <BreadcrumbItem>Event</BreadcrumbItem>
            <BreadcrumbItem active>Details</BreadcrumbItem>
          </Breadcrumb>
        </div>
        <Container>
          <ListGroup>
            <ListGroupItem color="info">
              {this.state.event.name}
            </ListGroupItem>
            {this.state.students.map((student) => (
              <ListGroupItem key={student}>
                {student}
              </ListGroupItem>
            ))}
          </ListGroup>
        </Container>
      </>
    )
  }
}

export default withRouter(Event);
