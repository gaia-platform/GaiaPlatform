import React from "react";
import {
  Breadcrumb,
  BreadcrumbItem,
  Container
} from "reactstrap";
import axios from "axios";
import { Calendar, momentLocalizer } from 'react-big-calendar'
import moment from 'moment'


export default class SchoolCalendar extends React.Component {
  state = {
    events: []
  }

  componentDidMount() {
    axios.get("/events").then((res) => {
      if (res.status === 200 && res.data) {
        this.setState({ events: res.data });
      }
    });
  }

  render() {
    const localizer = momentLocalizer(moment)
    return (
      <>
        <div>
          <Breadcrumb>
            <BreadcrumbItem><a href="/">Home</a></BreadcrumbItem>
            <BreadcrumbItem>Events</BreadcrumbItem>
            <BreadcrumbItem active>Calendar</BreadcrumbItem>
          </Breadcrumb>
        </div>
        <Container>
          <Calendar
            localizer={localizer}
            events={this.state.events}
            style={{ height: 500 }}
            titleAccessor="name"
            startAccessor={obj => new Date(obj.startTime * 1000)}
            endAccessor={obj => new Date(obj.endTime * 1000)}
            onSelectEvent={obj => {
              window.location.href = '/event/' + obj.id;
            }}
          />
        </Container>
      </>
    );
  }
}
