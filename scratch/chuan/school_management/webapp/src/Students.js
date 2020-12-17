import React from "react";
import { Breadcrumb, BreadcrumbItem, Container, Table } from "reactstrap";
import axios from "axios";

class Students extends React.Component {
  state = {
    students: [],
  };

  componentDidMount() {
    axios.get("/people/findByRole?role=student").then((res) => {
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
            <BreadcrumbItem>People</BreadcrumbItem>
            <BreadcrumbItem active>Students</BreadcrumbItem>
          </Breadcrumb>
        </div>
        <Container>
          <Table hover>
            <thead>
              <tr>
                <th>#</th>
                <th>First Name</th>
                <th>Last Name</th>
                <th>Student Id</th>
                <th>Face Signature</th>
              </tr>
            </thead>
            <tbody>
              {this.state.students.map((student, index) => (
                <tr key={index}>
                  <th scope="row">{index}</th>
                  <td>{student.firstName}</td>
                  <td>{student.lastName}</td>
                  <td>{student.studentId}</td>
                  <td>{student.faceSignature}</td>
                </tr>
              ))}
            </tbody>
          </Table>
        </Container>
      </>
    );
  }
}

export default Students;
