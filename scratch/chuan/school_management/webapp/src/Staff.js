import React from "react";
import { Breadcrumb, BreadcrumbItem, Container, Table } from "reactstrap";
import axios from "axios";

class Staff extends React.Component {
  state = {
    staff: [],
  };

  componentDidMount() {
    axios.get("/people/findByRole?role=staff").then((res) => {
      console.log(res);
      if (res.status === 200 && res.data) {
        const staff = res.data;
        this.setState({ staff });
      }
    });
  }

  render() {
    return (
      <>
        <div>
          <Breadcrumb>
            <BreadcrumbItem><a href="/">Home</a></BreadcrumbItem>
            <BreadcrumbItem>People</BreadcrumbItem>
            <BreadcrumbItem active>Staff</BreadcrumbItem>
          </Breadcrumb>
        </div>
        <Container>
          <Table hover>
            <thead>
              <tr>
                <th>#</th>
                <th>First Name</th>
                <th>Last Name</th>
                <th>Staff Id</th>
                <th>Face Signature</th>
              </tr>
            </thead>
            <tbody>
              {this.state.staff.map((staff, index) => (
                <tr key={index}>
                  <th scope="row">{index}</th>
                  <td>{staff.firstName}</td>
                  <td>{staff.lastName}</td>
                  <td>{staff.staffId}</td>
                  <td>{staff.faceSignature}</td>
                </tr>
              ))}
            </tbody>
          </Table>
        </Container>
      </>
    );
  }
}

export default Staff;
