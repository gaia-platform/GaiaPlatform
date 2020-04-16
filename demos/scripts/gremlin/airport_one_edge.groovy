import com.gaiaplatform.database.cachegraph.tinkerpop.gremlin.structure.CacheFactory

// Create tiny airport graph.
graph = CacheFactory.openWithAirportSupport()
CacheFactory.generateTinyQ1Airport(graph)
// CacheFactory.loadGraphml(graph, "gaia-airport-one-edge.graphml")
g = graph.traversal()

// Group airports by number of outgoing flights.
g.V().hasLabel('airport').
  group().by(out().count()).by('name').
  unfold().project('outgoing flights', 'airport').by(keys).by(values)
// By incoming flights.
g.V().hasLabel('airport').out().
  groupCount().by('name').order(local).by(values).unfold()

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
g.V().has('iata', 'SEA').repeat(out().as('hop')).times(2).has('iata', 'OTP').
  select(first, 'hop').dedup().values('name')

// Describe the 1 hop routes between SEA and OTP.
g.V().has('iata', 'SEA').repeat(outE().inV()).times(2).has('iata', 'OTP').
  path().by('iata').by('airline')

// Find all routes between SEA and OTP.
g.V().has('iata', 'SEA').repeat(outE().inV()).until(has('iata', 'OTP')).
  path().by('iata').by('airline').limit(24)

// Describe 1 stop routes between SEA and OTP that do not stop in UK.
g.V().has('iata', 'SEA').outE().inV().has('country', neq('United Kingdom')).
  outE().inV().has('iata', 'OTP').
  path().by('iata').by('airline')

// How many stops can there be in a flight.
g.E().not(has('stops', 0)).values('stops').dedup()

// Which are the airlines that fly flights with stops (get their codes because we can't get their name).
g.E().not(has('stops', 0)).values('airline').dedup()

// Which are the airlines that take me from Seattle to a place from which I can fly to Bucharest on a Tarom flight.
g.V().has('iata', 'SEA').outE().as('a').inV().outE().has('airline', 'RO').inV().has('iata', 'OTP').
  select('a').by('airline').dedup()
// Which are the cities reachable in 1 flight from SEA, from which I can reach Bucharest on a direct Tarom flight.
g.V().has('iata', 'SEA').out().as('a').outE().has('airline', 'RO').inV().has('iata', 'OTP').
  select('a').by('city').dedup()

// What are the flights from SEA to OTP that avoid flying with RO.
g.V().has('iata', 'SEA').repeat(outE().not(has('airline', 'RO')).inV()).times(2).has('iata', 'OTP').
  path().by('iata').by('airline')
