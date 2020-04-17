# Sample Gremlin queries for airport graphs

This folder contains sample Gremlin queries.

## airport_one_edge.groovy

These queries are meant to be used with graph data from *gaia-airport-one-edge.graphml*.

This is a schema in which airline nodes are not connected and there is just one type of edge: routes, which connect airport nodes:

* ```airport-->route-->airport```

## airport.groovy

These queries are meant to be used with graph data from *gaia-airport.graphml*.

This is a more complex schema, with connected airline nodes and three types of edges. Routes are represented as flight nodes and are linked to airports and airlines nodes via edges that carry no properties:

* ```airport-->departure->flight```

* ```flight-->arrives_at-->airport```

* ```flight-->operated_by-->airline```
