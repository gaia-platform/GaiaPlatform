import React from "react";
import { Breadcrumb, BreadcrumbItem, Container, Table } from "reactstrap";
import axios from "axios";

class Parents extends React.Component {
  state = {
    parents: [],
  };

  componentDidMount() {
    axios.get("/people/findByRole?role=parent").then((res) => {
      console.log(res);
      if (res.status === 200 && res.data) {
        const parents = res.data;
        this.setState({ parents });
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
            <BreadcrumbItem active>Parents</BreadcrumbItem>
          </Breadcrumb>
        </div>
        <Container>
          <Table hover>
            <thead>
              <tr>
                <th>#</th>
                <th>First Name</th>
                <th>Last Name</th>
                <th>Parent Id</th>
                <th>Face Signature</th>
              </tr>
            </thead>
            <tbody>
              {this.state.parents.map((parent, index) => (
                <tr key={index}>
                  <th scope="row">{index}</th>
                  <td>{parent.firstName}</td>
                  <td>{parent.lastName}</td>
                  <td>{parent.parentId}</td>
                  <td>{parent.faceSignature}</td>
                </tr>
              ))}
            </tbody>
          </Table>

        </Container>
      </>
    );
  }
}

export default Parents;
