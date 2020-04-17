# Airport data, from  openflights.com

## dat.gz files

These files represent the original airport data in CSV format. You can decompress them using the following command: ```gzip -dk <filename.dat.gz>```.

The *seattle_only* subfolder contains a subset of routes that only include flights to and from Seattle.

Data is denormalized.

### Airports.dat

| Field name | Description
| --- | ---
| Airport ID | Unique OpenFlights identifier for this airport.
| Name | Name of airport. May or may not contain the City name.
| City | Main city served by airport. May be spelled differently from Name.
| Country | Country or territory where airport is located. See countries.dat to cross-reference to ISO 3166-1 codes.
| IATA | 3-letter IATA code. Null if not assigned/unknown.
| ICAO | 4-letter ICAO code. Null if not assigned.
| Latitude | Decimal degrees, usually to six significant digits. Negative is South, positive is North.
| Longitude | Decimal degrees, usually to six significant digits. Negative is West, positive is East.
| Altitude | In feet.
| Timezone | Hours offset from UTC. Fractional hours are expressed as decimals, eg. India is 5.5.
| DST | Daylight savings time. One of E (Europe), A (US/Canada), S (South America), O (Australia), Z (New Zealand), N (None) or U (Unknown). See also: Help: Time
| Tz | database time zone. Timezone in "tz" (Olson) format, eg. "America/Los_Angeles".
| Type | Type of the airport. Value "airport" for air terminals, "station" for train stations, "port" for ferry terminals and "unknown" if not known. In airports.csv, only type=airport is included.
| Source | Source of this data. "OurAirports" for data sourced from OurAirports, "Legacy" for old data not matched to OurAirports (mostly DAFIF), "User" for unverified user contributions. In airports.csv, only source=OurAirports is included.<br>The data is UTF-8 encoded.

### Airlines.dat

| Field name | Description
| --- | ---
| Airline ID | Unique OpenFlights identifier for this airline.
| Name | Name of the airline.
| Alias | Alias of the airline. For example, All Nippon Airways is commonly known as "ANA".
| IATA | 2-letter IATA code, if available.
| ICAO | 3-letter ICAO code, if available.
| Callsign | Airline callsign.
| Country | Country or territory where airline is incorporated.
| Active | "Y" if the airline is or has until recently been operational, "N" if it is defunct. This field is not reliable: in particular, major airlines that stopped flying long ago, but have not had their IATA code reassigned (eg. Ansett/AN), will incorrectly show as "Y".<br>The data is UTF-8 encoded. The special value \N is used for "NULL" to indicate that no value is available, and is understood automatically by MySQL if imported.<br>Notes: Airlines with null codes/callsigns/countries generally represent user-added airlines. Since the data is intended primarily for current flights, defunct IATA codes are generally not included. For example, "Sabena" is not listed with a SN IATA code, since "SN" is presently used by its successor Brussels Airlines.

#### Sample entries

324,"All Nippon Airways","ANA All Nippon Airways","NH","ANA","ALL NIPPON","Japan","Y"

412,"Aerolineas Argentinas",\N,"AR","ARG","ARGENTINA","Argentina","Y"

413,"Arrowhead Airways",\N,"","ARH","ARROWHEAD","United States","N"

## graphml files

These files contain airport data in graphml format. Graphml is a standard format for graph data that can be processed with Gremlin and with other tools.

* The file *gaia-airport.graphml* was built from this data using the Java tool from *scratch/laurentiu/airport-graphml*. It only contains routes that could properly be resolved to airports and airlines. A bit over 1000 routes from the original dataset could not be resolved and were dropped. This file was tested to load into TinkerGraph using the command: ```graph.io(graphml()).readGraph('gaia-airport.graphml')```. It can be loaded into CacheGraph using the command ```graph = CacheFactory.loadGraphml('gaia-airport.graphml')``` or into an existing CacheGraph instance using the command ```CacheFactory.loadGraphml(graph, 'gaia-airport.graphml')```.

* *gaia-airport-tiny.graphml* uses the same graph schema as *gaia-airport.graphml*, but only includes data related to 5 airlines and 5 airports. This dataset is primarily meant to be used for testing, but can also be used for demos or for demonstrating Gremlin features.

* *gaia-airport-one-edge.graphml* uses a simpler graph schema with disconnected airline vertices and just one type of edges: routes.
