import React from "react";
import {
  Container,
  Card,
  Button,
  CardTitle,
  CardText,
  CardColumns,
  CardSubtitle,
  CardBody,
  Input,
  InputGroup,
  InputGroupAddon
} from "reactstrap";
import axios from "axios";

function ScanForm() {
  return (
    <InputGroup>
      <Input placeholder="enter a face signature..." />
      <InputGroupAddon addonType="append"><Button color="secondary">Scan</Button></InputGroupAddon>
    </InputGroup>
  );
}

function BuildingCard(props) {
  if (props.open) {
    return (
      <Card key={props.index}>
        <CardBody>
          <CardTitle tag="h5">{props.name}</CardTitle>
          <CardSubtitle tag="h6" className="mb-2 text-muted">
            Open
          </CardSubtitle>
          <CardText>
            The door will be closed shortly.
          </CardText>
        </CardBody>
      </Card>
    );
  } else {
    return (
      <Card key={props.index} body inverse style={{ backgroundColor: '#333', borderColor: '#333' }}>
        <CardBody>
          <CardTitle tag="h5">{props.name}</CardTitle>
          <CardSubtitle tag="h6" className="mb-2 text-muted">
            Closed
          </CardSubtitle>
          <CardText>
            <ScanForm />
          </CardText>
        </CardBody>
      </Card>
    )
  }
}

class Buildings extends React.Component {
  state = {
    buildings: [],
  };

  componentDidMount() {
    axios.get("/buildings").then((res) => {
      console.log(res);
      if (res.status === 200 && res.data) {
        const buildings = res.data;
        this.setState({ buildings });
      }
    });
  }

  render() {
    return (
      <Container>
        <h2>Buildings</h2>
        <hr />
        <div>
          <CardColumns>
            {this.state.buildings.map((building, index) => (
              <BuildingCard open={building.open} name={building.name} index={index} />
            ))}
          </CardColumns>
        </div>
      </Container >
    );
  }
}

export default Buildings;
