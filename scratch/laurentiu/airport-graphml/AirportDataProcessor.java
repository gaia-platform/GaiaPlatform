/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

import java.io.FileWriter;
import java.io.IOException;

import java.text.ParseException;

import java.time.ZonedDateTime;
import java.util.HashMap;

public class AirportDataProcessor
{
    protected static final String AIRPORTS_FILENAME = "airports.dat";
    protected static final String AIRLINES_FILENAME = "airlines.dat";
    protected static final String ROUTES_FILENAME = "routes.dat";

    protected static final String OUTPUT_FILENAME = "gaia-airport.graphml";

    protected static final int AIRPORTS_COLUMN_COUNT = 14;
    protected static final int AIRLINES_COLUMN_COUNT = 8;
    protected static final int ROUTES_COLUMN_COUNT = 9;

    protected static int globalRecordId = 0;

    protected static boolean printDebuggingOutput = false;

    protected static FileWriter output;

    protected static HashMap<String, Integer> airportIataMap = new HashMap<>();
    protected static HashMap<String, Integer> airlineIataMap = new HashMap<>();

    protected static HashMap<Integer, Integer> airportIdMap = new HashMap<>();
    protected static HashMap<Integer, Integer> airlineIdMap = new HashMap<>();

    protected enum DataType
    {
        STRING,
        INTEGER,
        FLOAT,
        DOUBLE,
    }

    protected class AirportIndex
    {
        protected static final int ID = 0;
        protected static final int NAME = 1;
        protected static final int CITY = 2;
        protected static final int COUNTRY = 3;
        protected static final int IATA = 4;
        protected static final int ICAO = 5;
        protected static final int LATITUDE = 6;
        protected static final int LONGITUDE = 7;
        protected static final int ALTITUDE = 8;
        protected static final int TIMEZONE = 9;
        protected static final int TZTEXT = 10;
        protected static final int DST = 11;
        protected static final int TYPE = 12;
        protected static final int SOURCE = 13;
    }

    protected class AirlineIndex
    {
        protected static final int ID = 0;
        protected static final int NAME = 1;
        protected static final int ALIAS = 2;
        protected static final int IATA = 3;
        protected static final int ICAO = 4;
        protected static final int CALLSIGN = 5;
        protected static final int COUNTRY = 6;
        protected static final int ACTIVE = 7;
    }

    protected class RouteIndex
    {
        protected static final int AIRLINE_IATA = 0;
        protected static final int AIRLINE_ID = 1;
        protected static final int DEPARTURE_AIRPORT_IATA = 2;
        protected static final int DEPARTURE_AIRPORT_ID = 3;
        protected static final int ARRIVAL_AIRPORT_IATA = 4;
        protected static final int ARRIVAL_AIRPORT_ID = 5;
        protected static final int CODESHARE = 6;
        protected static final int STOPS = 7;
        protected static final int EQUIPMENT = 8;
    }

    public static void main(String[] args) throws IOException
    {
        if (args.length > 0)
        {
            System.out.println("Presence of an argument triggers debugging output.");
            printDebuggingOutput = true;
        }

        output = new FileWriter(OUTPUT_FILENAME);

        output.write("<?xml version='1.0' ?>\n");
        output.write("\n");
        output.write("<!--airport data in graphml format-->\n");
        output.write("\n");
        output.write("<!--Copyright (c) Gaia Platform LLC-->\n");
        output.write("<!--All rights reserved.-->\n");
        output.write("\n");
        output.write("<!--Generated on: " + ZonedDateTime.now() + ".-->\n");
        output.write("\n");
        output.write("<graphml xmlns='http://graphml.graphdrawing.org/xmlns'>\n");
        output.write("  <key id='ap_id' for='node' attr.name='ap_id' attr.type='int'/>\n");
        output.write("  <key id='al_id' for='node' attr.name='al_id' attr.type='int'/>\n");
        output.write("  <key id='name' for='node' attr.name='name' attr.type='string'/>\n");
        output.write("  <key id='alias' for='node' attr.name='alias' attr.type='string'/>\n");
        output.write("  <key id='callsign' for='node' attr.name='callsign' attr.type='string'/>\n");
        output.write("  <key id='active' for='node' attr.name='active' attr.type='string'/>\n");
        output.write("  <key id='iata' for='node' attr.name='iata' attr.type='string'/>\n");
        output.write("  <key id='icao' for='node' attr.name='icao' attr.type='string'/>\n");
        output.write("  <key id='country' for='node' attr.name='country' attr.type='string'/>\n");
        output.write("  <key id='city' for='node' attr.name='city' attr.type='string'/>\n");
        output.write("  <key id='latitude' for='node' attr.name='latitude' attr.type='double'/>\n");
        output.write("  <key id='longitude' for='node' attr.name='longitude' attr.type='double'/>\n");
        output.write("  <key id='altitude' for='node' attr.name='altitude' attr.type='int'/>\n");
        output.write("  <key id='timezone' for='node' attr.name='timezone' attr.type='float'/>\n");
        output.write("  <key id='tztext' for='node' attr.name='tztext' attr.type='string'/>\n");
        output.write("  <key id='dst' for='node' attr.name='dst' attr.type='string'/>\n");
        output.write("  <key id='source' for='node' attr.name='source' attr.type='string'/>\n");
        output.write("  <key id='equipment' for='node' attr.name='equipment' attr.type='string'/>\n");
        output.write("  <key id='codeshare' for='node' attr.name='codeshare' attr.type='string'/>\n");
        output.write("  <key id='stops' for='node' attr.name='stops' attr.type='int'/>\n");
        output.write("  <key id='labelV' for='node' attr.name='labelV' attr.type='string'/>\n");
        output.write("  <key id='labelE' for='edge' attr.name='labelE' attr.type='string'/>\n");
        output.write("\n");
        output.write("  <graph id='gaia-airport' edgedefault='directed'>\n");

        try
        {
            processFiles();
        }
        catch (Exception e)
        {
            System.out.println(e.getMessage());
        }
        
        output.write("</graph>\n");
        output.write("</graphml>\n");
        output.close();
    }

    protected static void processFiles() throws IOException, ParseException
    {
        System.out.println("\n>>> Processing airports.dat...");
        output.write("\n");
        output.write("<!--####### AIRPORTS #######-->\n");
        output.write("\n");
        processAirports();

        System.out.println("\n>>> Processing airlines.dat...");
        output.write("\n");
        output.write("<!--####### AIRLINES #######-->\n");
        output.write("\n");
        processAirlines();

        System.out.println("\n>>> Processing routes.dat...");
        output.write("\n");
        output.write("<!--####### FLIGHTS #######-->\n");
        output.write("\n");
        processRoutes();
    }

    protected static void outputData(String data, String name) throws IOException
    {
        outputData(data, name, DataType.STRING);
    }

    protected static void outputData(String data, String name, DataType type) throws IOException
    {
        if (data.isEmpty())
        {
            return;
        }

        switch (type)
        {
            case STRING:
            output.write("    <data key='" + name + "'>" + data + "</data>\n");
            break;

            case INTEGER:
            output.write("    <data key='" + name + "'>" + Integer.parseInt(data) + "</data>\n");
            break;

            case FLOAT:
            output.write("    <data key='" + name + "'>" + Float.parseFloat(data) + "</data>\n");
            break;

            case DOUBLE:
            output.write("    <data key='" + name + "'>" + Double.parseDouble(data) + "</data>\n");
            break;
        }    
    }

    protected static String cleanupString(String data)
    {
        if (data.isEmpty())
        {
            return data;
        }

        String newData = data;

        // Some strings contain control characters
        // that don't work well with XML.
        // As we don't have an encoding method,
        // we'll just have to look them up and remove them.
        if (data.length() != data.getBytes().length)
        {
            StringBuilder newString = new StringBuilder();

            for (int i = 0; i < data.length(); i++)
            {
                char character = data.charAt(i);
                if (Character.isISOControl(character))
                {
                    // Skip the control character as well as the next two characters.
                    i += 2;
                    continue;
                }
                
                newString.append(character);
            }
    
            newData = newString.toString();
        }

        // We also need to escape ampersands.
        newData = newData.replace("&", "&amp;");

        if (!newData.equals(data) && printDebuggingOutput)
        {
            System.out.println("Changed [" + data + "] to [" + newData + "].");
        }

        return newData;
    }

    // Map IATA and record id to an entity id
    // so we can later look them up to generate the edge information.
    protected static int mapIdentifiers(
        int entityId,
        String iata, HashMap<String, Integer> iataMap,
        String id, HashMap<Integer, Integer> idMap,
        int lineCount)
    {
        boolean hasMappedIata = false;

        // Map IATA code if one is present and has not been seen before.
        if (!iata.isEmpty())
        {
            if (iataMap.containsKey(iata))
            {
                entityId = iataMap.get(iata);

                System.out.println(
                    "WARNING: A duplicate IATA [" + iata
                    + "] was found on line " + lineCount
                    + "! Will try to map the numerical id of this record to the previously assigned entity id: "
                    + entityId + ".");
            }
            else
            {
                iataMap.put(iata, entityId);
            }

            hasMappedIata = true;
        }

        // Map id.
        int integerId = 0;
        try
        {
            integerId = Integer.parseInt(id);
        }
        catch (Exception e)
        {
            System.out.println(
                "WARNING: An invalid id [" + id
                + "] was found on line " + lineCount
                + "! Will use IATA mapping or will skip this record if IATA could not be mapped.");

            return hasMappedIata ? entityId : 0;
        }

        if (idMap.containsKey(integerId))
        {
            System.out.println(
                "WARNING: A duplicate id [" + id
                + "] was found on line " + lineCount
                + "! Will use IATA mapping or will skip this record if IATA could not be mapped.");

            return hasMappedIata ? entityId : 0;
        }
        else
        {
            idMap.put(integerId, entityId);
        }

        return entityId;
    }

    protected static int lookupIdentifiers(
        String iata, HashMap<String, Integer> iataMap,
        String id, HashMap<Integer, Integer> idMap)
    {
        // Lookup the IATA code.
        if (!iata.isEmpty())
        {
            if (iataMap.containsKey(iata))
            {
                return iataMap.get(iata);
            }
        }

        // Lookup the id.
        if (!id.isEmpty())
        {
            int integerId = Integer.parseInt(id);
            if (idMap.containsKey(integerId))
            {
                return idMap.get(integerId);
            }
        }

        return 0;
    }

    protected static void processAirports() throws IOException, ParseException
    {
        CsvParser parser = new CsvParser(AIRPORTS_FILENAME, AIRPORTS_COLUMN_COUNT);
        String[] columns;
        int countNodes = 0;

        while ((columns = parser.getNextColumns()) != null)
        {
            int airportNodeId = ++globalRecordId;

            String airportIata = columns[AirportIndex.IATA];
            String airportId = columns[AirportIndex.ID];
            airportNodeId = mapIdentifiers(
                airportNodeId,
                airportIata, airportIataMap,
                airportId, airportIdMap,
                parser.getLineCount());
            if (airportNodeId == 0)
            {
                continue;
            }

            if (!columns[AirportIndex.TYPE].equals("airport"))
            {
                System.out.println(
                    "WARNING: An unexpected airport type [" + columns[AirportIndex.TYPE]
                    + "] was encountered on line " + parser.getLineCount() + "!");
            }

            output.write("  <node id='" + airportNodeId + "'>\n");
            output.write("    <data key='labelV'>airport</data>\n");
            outputData(columns[AirportIndex.ID], "ap_id", DataType.INTEGER);
            outputData(cleanupString(columns[AirportIndex.NAME]), "name");
            outputData(columns[AirportIndex.CITY], "city");
            outputData(columns[AirportIndex.COUNTRY], "country");
            outputData(columns[AirportIndex.IATA], "iata");
            outputData(columns[AirportIndex.ICAO], "icao");
            outputData(columns[AirportIndex.LATITUDE], "latitude", DataType.DOUBLE);
            outputData(columns[AirportIndex.LONGITUDE], "longitude", DataType.DOUBLE);
            outputData(columns[AirportIndex.ALTITUDE], "altitude", DataType.INTEGER);
            outputData(columns[AirportIndex.TIMEZONE], "timezone");
            outputData(columns[AirportIndex.TZTEXT], "tztext");
            outputData(columns[AirportIndex.DST], "dst");
            outputData(columns[AirportIndex.SOURCE], "source");
            output.write("  </node>\n");

            if (printDebuggingOutput)
            {
                System.out.println("airport " + airportIata + "(" + airportId + ") => " + airportNodeId);
            }

            ++countNodes;
        }

        System.out.println(
            "Processed " + parser.getLineCount() + " airport records and generated "
            + countNodes + " nodes.");
    }    

    protected static void processAirlines() throws IOException, ParseException
    {
        CsvParser parser = new CsvParser(AIRLINES_FILENAME, AIRLINES_COLUMN_COUNT);
        String[] columns;
        int countNodes = 0;

        while ((columns = parser.getNextColumns()) != null)
        {
            int airlineNodeId = ++globalRecordId;

            String airlineIata = columns[AirlineIndex.IATA];
            String airlineId = columns[AirlineIndex.ID];
            airlineNodeId = mapIdentifiers(
                airlineNodeId,
                airlineIata, airlineIataMap,
                airlineId, airlineIdMap,
                parser.getLineCount());
            if (airlineNodeId == 0)
            {
                continue;
            }

            output.write("  <node id='" + airlineNodeId + "'>\n");
            output.write("    <data key='labelV'>airline</data>\n");
            outputData(columns[AirlineIndex.ID], "al_id", DataType.INTEGER);
            outputData(cleanupString(columns[AirlineIndex.NAME]), "name");
            outputData(columns[AirlineIndex.ALIAS], "alias");
            outputData(cleanupString(columns[AirlineIndex.IATA]), "iata");
            outputData(cleanupString(columns[AirlineIndex.ICAO]), "icao");
            outputData(cleanupString(columns[AirlineIndex.CALLSIGN]), "callsign");
            outputData(columns[AirlineIndex.COUNTRY], "country");
            outputData(columns[AirlineIndex.ACTIVE], "active");
            output.write("  </node>\n");

            if (printDebuggingOutput)
            {
                System.out.println("airline " + airlineIata + "(" + airlineId + ") => " + airlineNodeId);
            }

            ++countNodes;
        }

        System.out.println(
            "Processed " + parser.getLineCount() + " airline records and generated "
            + countNodes + " nodes.");
    }

    protected static void processRoutes() throws IOException, ParseException
    {
        CsvParser parser = new CsvParser(ROUTES_FILENAME, ROUTES_COLUMN_COUNT);
        String[] columns;
        int countEdges = 0;

        while ((columns = parser.getNextColumns()) != null)
        {
            String airlineIata = columns[RouteIndex.AIRLINE_IATA];
            String departureAirportIata = columns[RouteIndex.DEPARTURE_AIRPORT_IATA];
            String arrivalAirportIata = columns[RouteIndex.ARRIVAL_AIRPORT_IATA];

            String airlineId = columns[RouteIndex.AIRLINE_ID];
            String departureAirportId = columns[RouteIndex.DEPARTURE_AIRPORT_ID];
            String arrivalAirportId = columns[RouteIndex.ARRIVAL_AIRPORT_ID];

            if (printDebuggingOutput)
            {
                System.out.println(
                    "flight record: airline = " + airlineIata + "(" + airlineId + ")"
                    + " departure airport = " + departureAirportIata + "(" + departureAirportId + ")"
                    + " arrival airport = " + arrivalAirportIata + "(" + arrivalAirportId + ").");
            }

            int airlineNodeId = lookupIdentifiers(
                airlineIata, airlineIataMap, airlineId, airlineIdMap);
            int departureAirportNodeId = lookupIdentifiers(
                departureAirportIata, airportIataMap, departureAirportId, airportIdMap);
            int arrivalAirportNodeId = lookupIdentifiers(
                arrivalAirportIata, airportIataMap, arrivalAirportId, airportIdMap);

            if (airlineNodeId == 0)
            {
                System.out.println(
                    "WARNING: Could not resolve airline ["
                    + airlineIata + "(" + airlineId + ")]"
                    + " on line " + parser.getLineCount() + "! Will skip record.");
                continue;
            }
            if (departureAirportNodeId == 0)
            {
                System.out.println(
                    "WARNING: Could not resolve departure airport ["
                    + departureAirportIata + "(" + departureAirportId + ")]"
                    + " on line " + parser.getLineCount() + "! Will skip record.");
                continue;
            }
            if (arrivalAirportNodeId == 0)
            {
                System.out.println(
                    "WARNING: Could not resolve arrival airport ["
                    + arrivalAirportIata + "(" + arrivalAirportId + ")]"
                    + " on line " + parser.getLineCount() + "! Will skip record.");
                continue;
            }

            int flightNodeId = ++globalRecordId;

            // Output flight node.
            output.write("  <node id='" + flightNodeId + "'>\n");

            output.write("    <data key='labelV'>flight</data>\n");
            outputData(columns[RouteIndex.CODESHARE], "codeshare");
            outputData(columns[RouteIndex.STOPS], "stops", DataType.INTEGER);
            outputData(columns[RouteIndex.EQUIPMENT], "equipment");
            
            output.write("  </node>\n");

            // Output departure edge.
            int departureEdgeId = ++globalRecordId;

            output.write(
                "  <edge id='" + departureEdgeId
                + "' source='" + departureAirportNodeId + "' target = '" + flightNodeId + "'>\n");
            output.write("    <data key='labelE'>departure</data>\n");
            output.write("  </edge>\n");

            // Output arrives_at edge.
            int arrivesAtEdgeId = ++globalRecordId;

            output.write(
                "  <edge id='" + arrivesAtEdgeId
                + "' source='" + flightNodeId + "' target = '" + arrivalAirportNodeId + "'>\n");
            output.write("    <data key='labelE'>arrives_at</data>\n");
            output.write("  </edge>\n");

            // Output operated_by edge.
            int operatedByEdgeId = ++globalRecordId;

            output.write(
                "  <edge id='" + operatedByEdgeId
                + "' source='" + flightNodeId + "' target = '" + airlineNodeId + "'>\n");
            output.write("    <data key='labelE'>operated_by</data>\n");
            output.write("  </edge>\n");

            if (printDebuggingOutput)
            {
                System.out.println("flight => " + flightNodeId);
                System.out.println("    departure airport " + departureAirportIata + "(" + departureAirportId + ") => " + departureAirportNodeId);
                System.out.println("    arrival airport " + arrivalAirportIata + "(" + arrivalAirportId + ") => " + arrivalAirportNodeId);
                System.out.println("    airline " + airlineIata + "(" + airlineId + ") => " + airlineNodeId);
                System.out.println("    departure => " + departureEdgeId);
                System.out.println("    arrives_at => " + arrivesAtEdgeId);
                System.out.println("    operated_by => " + operatedByEdgeId);
            }

            ++countEdges;
        }

        System.out.println(
            "Processed " + parser.getLineCount() + " route records and generated "
            + countEdges + " edges.");
    }
}
