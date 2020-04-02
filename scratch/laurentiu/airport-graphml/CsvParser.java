/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

import java.lang.StringBuilder;

import java.text.ParseException;

import java.util.ArrayList;

public class CsvParser
{
    protected static final String EMPTY_STRING = "";
    protected static final String COLUMN_SEPARATOR = ",";
    protected static final String QUOTE = "\"";
    protected static final String DOUBLE_QUOTE = "\"\"";
    protected static final String NULL_VALUE = "\\N";

    protected BufferedReader reader;
    protected int lineCount;
    protected int columnCount;

    public CsvParser(String filename, int columnCount) throws IOException
    {
        this.reader = new BufferedReader(new FileReader(filename));
        this.lineCount = 0;
        this.columnCount = columnCount;
    }

    public int getLineCount()
    {
        return this.lineCount;
    }

    public String[] getNextColumns() throws IOException, ParseException
    {
        String line = reader.readLine();

        if (line == null)
        {
            return null;
        }

        ++this.lineCount;
        
        String[] columns = line.split(COLUMN_SEPARATOR);

        try
        {
            // If the line ended with an empty element,
            // we need to add it back.
            // Let the fixing code know about it.
            columns = fixColumns(columns, line.endsWith(COLUMN_SEPARATOR));
        }
        catch (Exception e)
        {
            handleBadLine(this.lineCount, line, e);
        }

        // If our fixes did not succeed, the line may be badly formatted.
        if (columns.length != this.columnCount)
        {
            handleBadLine(this.lineCount, line);
        }

        return columns;
    }

    protected static void handleBadLine(int lineCount, String line) throws ParseException
    {
        handleBadLine(lineCount, line, null);
    }

    protected static void handleBadLine(int lineCount, String line, Exception e) throws ParseException
    {
        // Collect information to allow us to debug the failure.
        StringBuilder errorMessage = new StringBuilder();
        errorMessage.append("\n>>> Error detected on line ");
        errorMessage.append(lineCount);
        errorMessage.append(":\n");
        errorMessage.append(line);

        if (e != null)
        {
            errorMessage.append("\n>>> Exception message: ");
            errorMessage.append(e.getMessage());

            // Parse exceptions contain additional information.
            if (e instanceof ParseException)
            {
                errorMessage.append("\n>>> Error offset: ");
                errorMessage.append(((ParseException)e).getErrorOffset());
            }
        }

        throw new ParseException(errorMessage.toString(), 0);
    }

    // This logic does not handle all valid CSV content,
    // but is good enough for the content it had to process.
    // It needs to be fixed to properly address situations
    // when escaped quotes are next to enclosing quotes or to enclosed commas.
    protected static String[] fixColumns(String[] columns, boolean addEmptyColumn) throws ParseException
    {
        ArrayList<String> fixedColumns = new ArrayList<>();
        StringBuilder fixedColumnBuilder = null;

        for (int i = 0; i < columns.length; i++)
        {
            if (columns[i].length() > 1 && columns[i].startsWith(QUOTE))
            {
                if (fixedColumnBuilder != null)
                {
                    StringBuilder errorMessage = new StringBuilder();
                    errorMessage.append("Unexpected column continuation: ");
                    errorMessage.append(columns[i]);
                    throw new ParseException(errorMessage.toString(), 0);
                }

                if (columns[i].endsWith(QUOTE))
                {
                    fixedColumns.add(decodeColumnValue(columns[i]));
                }
                else
                {
                    fixedColumnBuilder = new StringBuilder();
                    fixedColumnBuilder.append(columns[i]);
                    fixedColumnBuilder.append(COLUMN_SEPARATOR);
                }
            }
            else if (fixedColumnBuilder != null)
            {
                fixedColumnBuilder.append(columns[i]);
                if (columns[i].endsWith(QUOTE))
                {
                    fixedColumns.add(decodeColumnValue(fixedColumnBuilder.toString()));
                    fixedColumnBuilder = null;
                }
                else
                {
                    fixedColumnBuilder.append(COLUMN_SEPARATOR);
                }
            }
            else
            {
                if (columns[i].endsWith(QUOTE))
                {
                    StringBuilder errorMessage = new StringBuilder();
                    errorMessage.append("Unexpected column continuation: ");
                    errorMessage.append(columns[i]);
                    throw new ParseException(errorMessage.toString(), columns[i].length() - 1);
                }

                fixedColumns.add(decodeColumnValue(columns[i]));
            }
        }

        if (addEmptyColumn)
        {
            fixedColumns.add(EMPTY_STRING);
        }

        return fixedColumns.toArray(new String[fixedColumns.size()]);
    }

    protected static String decodeColumnValue(String column) throws ParseException
    {
        String newColumn = column;

        // If the column value was quoted, remove those quotes.
        if (column.length() > 1 && column.startsWith(QUOTE))
        {
            if (!column.endsWith(QUOTE))
            {
                throw new ParseException("Column is not properly quoted: " + column, column.length() - 1);
            }

            if (column.length() > 2)
            {
                newColumn = column.substring(1, column.length() - 1);
            }
            else
            {
                newColumn = EMPTY_STRING;
            }
        }

        // Remove the escaping of quotes inside the string.
        newColumn = newColumn.replaceAll(DOUBLE_QUOTE, QUOTE);

        // Process null values as empty strings - works for our case.
        if (newColumn.equals(NULL_VALUE))
        {
            newColumn = EMPTY_STRING;
        }

        return newColumn;
    }
}
