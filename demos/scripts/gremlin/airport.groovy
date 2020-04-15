import com.gaiaplatform.database.cachegraph.tinkerpop.gremlin.structure.CacheGraph
import com.gaiaplatform.database.cachegraph.tinkerpop.gremlin.structure.CacheFactory

// Create tiny airport graph.
graph = CacheGraph.open()
CacheFactory.generateTinyAirport(graph)
// CacheFactory.loadGraphml(graph, "gaia-airport.graphml")
g = graph.traversal()

// Group airports by number of outgoing flights.
g.V().hasLabel('airport').
  group().by(out().count()).by('name').
  unfold().project('outgoing flights', 'airport').by(keys).by(values)
// By incoming flights.
g.V().hasLabel('airport').out().out('arrives_at').
  groupCount().by('name').order(local).by(values).unfold()

// How many airports are there by country.
g.V().hasLabel('airport').groupCount().by('country')

// Find all the airports in US.
g.V().hasLabel('airport').has('country', 'USA').values('name')

// Find all airports directly reachable from Seattle.
g.V().has('iata', 'SEA').out().out('arrives_at').dedup().values('name')

// Find all airports reachable in 1 hop from Seattle.
g.V().has('iata', 'SEA').repeat(out().out('arrives_at')).times(2).dedup().values('name')

// Is there a direct flight from SEA to OTP?
g.V().has('iata', 'SEA').out().out('arrives_at').has('iata', 'OTP').hasNext()

// Find the airports where you'd have to change the flight, to get from SEA to OTP.
g.V().has('iata', 'SEA').repeat(out().out('arrives_at').as('hop')).times(2).has('iata', 'OTP').
  select(first, 'hop').dedup().values('name')

// Describe the 1 hop routes between SEA and OTP.
g.V().has('iata', 'SEA').repeat(out().out('arrives_at')).times(2).has('iata', 'OTP').
  path().by('iata').by(out('operated_by').values('icao'))

// Find all routes between SEA and OTP.
g.V().has('iata', 'SEA').repeat(out().out('arrives_at')).until(has('iata', 'OTP')).
  path().by('iata').by(out('operated_by').values('icao')).limit(24)

// Describe 1 stop routes between SEA and OTP that do not stop in UK.
g.V().has('iata', 'SEA').out().out('arrives_at').has('country', neq('United Kingdom')).
  out().out('arrives_at').has('iata', 'OTP').
  path().by('iata').by(out('operated_by').values('icao'))

// How many stops can there be in a flight.
g.V().hasLabel('flight').not(has('stops', 0)).values('stops').dedup()

// Which are the airlines that fly flights with stops.
g.V().hasLabel('flight').not(has('stops', 0)).out('operated_by').dedup().values('name')

// Which are the airlines that take me from Seattle to a place from which I can fly to Bucharest on a Tarom flight.
g.V().has('iata', 'SEA').out().as('f').out('arrives_at').
  out().where(out('operated_by').has('iata', eq('RO'))).out('arrives_at').has('iata', 'OTP').
  select('f').by(out('operated_by').values('name')).dedup()
// Which are the cities reachable in 1 flight from SEA, from which I can reach Bucharest on a direct Tarom flight.
g.V().has('iata', 'SEA').out().out('arrives_at').as('a').
  out().where(out('operated_by').has('iata', eq('RO'))).out('arrives_at').has('iata', 'OTP').
  select('a').by('city').dedup()

// What are the flights from SEA to OTP that avoid flying with RO.
g.V().has('iata', 'SEA').
  repeat(out().where(out('operated_by').not(has('iata', 'RO'))).out('arrives_at')).
  times(2).has('iata', 'OTP').
  path().by('iata').by(out('operated_by').values('icao'))
