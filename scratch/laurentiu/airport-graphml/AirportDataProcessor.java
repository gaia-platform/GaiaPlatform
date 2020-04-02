/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

import java.io.FileWriter;
import java.io.IOException;

import java.text.ParseException;

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

    protected static final String NULL_VALUE = "\\N";

    protected static int globalRecordId = 0;

    protected static FileWriter output;

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
        output = new FileWriter(OUTPUT_FILENAME);

        output.write("<?xml version='1.0' ?>\n");
        output.write("\n");
        output.write("<!--airport data in graphml format-->\n");
        output.write("<!--Copyright (c) Gaia Platform LLC-->\n");
        output.write("<!--All rights reserved.-->\n");
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
        output.write("\n");
        output.write("<!--####### AIRPORTS #######-->\n");
        output.write("\n");
        processAirports();
        output.write("\n");
        output.write("<!--####### AIRLINES #######-->\n");
        output.write("\n");
        processAirlines();
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
        if (data.equals(NULL_VALUE))
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

    protected static void processAirports() throws IOException, ParseException
    {
        CsvParser parser = new CsvParser(AIRPORTS_FILENAME, AIRPORTS_COLUMN_COUNT);
        String[] columns;

        while ((columns = parser.getNextColumns()) != null)
        {
            ++globalRecordId;

            output.write("  <node id='" + globalRecordId + "'>\n");

            output.write("    <data key='labelV'>airport</data>\n");
            outputData(columns[AirportIndex.ID], "ap_id", DataType.INTEGER);
            outputData(columns[AirportIndex.NAME], "name");
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

            if (!columns[AirportIndex.TYPE].equals("airport"))
            {
                StringBuilder errorMessage = new StringBuilder();
                errorMessage.append("Unexpected airport type [");
                errorMessage.append(columns[AirportIndex.TYPE]);
                errorMessage.append("] was encountered on line: ");
                errorMessage.append(globalRecordId);
                throw new ParseException(errorMessage.toString(), 0);
            }

            int airportId = Integer.parseInt(columns[AirportIndex.ID]);
            airportIdMap.put(airportId, globalRecordId);
            
            output.write("  </node>\n");
        }

        System.out.println("Processed " + parser.getLineCount() + " airport records.");
    }    

    protected static void processAirlines() throws IOException, ParseException
    {
        CsvParser parser = new CsvParser(AIRLINES_FILENAME, AIRLINES_COLUMN_COUNT);
        String[] columns;

        while ((columns = parser.getNextColumns()) != null)
        {
            ++globalRecordId;

            output.write("  <node id='" + globalRecordId + "'>\n");

            output.write("    <data key='labelV'>airline</data>\n");
            outputData(columns[AirlineIndex.ID], "ap_id", DataType.INTEGER);
            outputData(columns[AirlineIndex.NAME], "name");
            outputData(columns[AirlineIndex.ALIAS], "alias");
            outputData(columns[AirlineIndex.IATA], "iata");
            outputData(columns[AirlineIndex.ICAO], "icao");
            outputData(columns[AirlineIndex.CALLSIGN], "callsign");
            outputData(columns[AirlineIndex.COUNTRY], "country");
            outputData(columns[AirlineIndex.ACTIVE], "active");

            int airlineId = Integer.parseInt(columns[AirlineIndex.ID]);
            airlineIdMap.put(airlineId, globalRecordId);
            
            output.write("  </node>\n");
        }

        System.out.println("Processed " + parser.getLineCount() + " airline records.");
    }

    protected static void processRoutes() throws IOException, ParseException
    {
        CsvParser parser = new CsvParser(ROUTES_FILENAME, ROUTES_COLUMN_COUNT);
        String[] columns;

        while ((columns = parser.getNextColumns()) != null)
        {
            ++globalRecordId;

            output.write("  <node id='" + globalRecordId + "'>\n");

            output.write("    <data key='labelV'>flight</data>\n");
            outputData(columns[RouteIndex.CODESHARE], "codeshare");
            outputData(columns[RouteIndex.STOPS], "stops", DataType.INTEGER);
            outputData(columns[RouteIndex.EQUIPMENT], "equipment");
            
            output.write("  </node>\n");
        }

        System.out.println("Processed " + parser.getLineCount() + " route records.");
    }
}
