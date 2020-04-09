// Create tiny airport graph.
graph = com.gaiaplatform.database.cachegraph.tinkerpop.gremlin.structure.CacheFactory.createTinyQ1Airport()
g = graph.traversal()

// Group airports by number of outgoing flights.
g.V().hasLabel('airport').group().by(out().count()).by('name').unfold().project('outgoing flights', 'airport').by(keys).by(values)
// By incoming flights.
g.V().hasLabel('airport').out().groupCount().by('name').order(local).by(values).unfold()

// How many airports are there by country.
g.V().hasLabel('airport').groupCount().by('country')

// Find all the airports in US.
g.V().hasLabel('airport').has('country', 'USA').values('name')

// Find all airports directly reachable from Seattle.
g.V().has('iata', 'SEA').out().dedup().values('name')

// Find all airports reachable in 1 hop from Seattle.
g.V().has('iata', 'SEA').repeat(out()).times(2).dedup().values('name')

// Is there a direct flight from SEA to OTP?
g.V().has('iata', 'SEA').out().has('iata', 'OTP').hasNext()

// Find the airports where you'd have to change the flight, to get from SEA to OTP.
g.V().has('iata', 'SEA').repeat(out().as('hop')).times(2).has('iata', 'OTP').select(first, 'hop').dedup().values('name')

// Describe the 1 hop routes between SEA and OTP.
g.V().has('iata', 'SEA').repeat(outE().inV()).times(2).has('iata', 'OTP').path().by('iata').by('icao')

